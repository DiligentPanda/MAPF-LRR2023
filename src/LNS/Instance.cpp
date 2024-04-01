#include<boost/tokenizer.hpp>
#include <algorithm>    // std::shuffle
#include <random>      // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include "LNS/Instance.h"

namespace LNS {

int RANDOM_WALK_STEPS = 100000;


Instance::Instance(const SharedEnvironment & env) {
	// load map
	num_of_rows=env.rows;
	num_of_cols=env.cols;
	map_size=num_of_rows*num_of_cols;
	my_map.resize(map_size, false);

	for (int i=0;i<map_size;++i){
		my_map[i]=(bool)env.map[i];
	}

	// load agents
	num_of_agents=env.num_of_agents;
	start_locations.resize(num_of_agents);
	start_orientations.resize(num_of_agents);
	goal_locations.resize(num_of_agents);
	for (int i=0;i<num_of_agents;++i){
		start_locations[i]=env.curr_states[i].location;
		start_orientations[i]=env.curr_states[i].orientation;
		goal_locations[i]=env.goal_locations[i][0].first;
	}
}

void Instance::set_starts_and_goals(const SharedEnvironment & env) {
	for (int i=0;i<num_of_agents;++i){
		start_locations[i]=env.curr_states[i].location;
		start_orientations[i]=env.curr_states[i].orientation;
		goal_locations[i]=env.goal_locations[i][0].first;
	}
}

void Instance::set_starts_and_goals(const std::vector<::State> & starts, const std::vector<::State> & goals) {
	for (int i=0;i<num_of_agents;++i){
		start_locations[i]=starts[i].location;
		start_orientations[i]=starts[i].orientation;
		goal_locations[i]=goals[i].location;
	}
}

Instance::Instance(const string& map_fname, const string& agent_fname, 
	int num_of_agents, int num_of_rows, int num_of_cols, int num_of_obstacles, int warehouse_width):
	map_fname(map_fname), agent_fname(agent_fname), num_of_agents(num_of_agents)
{
	bool succ = loadMap();
	if (!succ)
	{
		if (num_of_rows > 0 && num_of_cols > 0 && num_of_obstacles >= 0 && 
			num_of_obstacles < num_of_rows * num_of_cols) // generate random grid
		{
			generateConnectedRandomGrid(num_of_rows, num_of_cols, num_of_obstacles);
			saveMap();
		}
		else
		{
			cerr << "Map file " << map_fname << " not found." << endl;
			exit(-1);
		}
	}

	succ = loadAgents();
	if (!succ)
	{
		if (num_of_agents > 0)
		{
			generateRandomAgents(warehouse_width);
			// saveAgents();
            saveNathan();
		}
		else
		{
			cerr << "Agent file " << agent_fname << " not found." << endl;
			exit(-1);
		}
	}

}


int Instance::randomWalk(int curr, int steps) const
{
	for (int walk = 0; walk < steps; walk++)
	{
		list<int> l = getNeighbors(curr);
		vector<int> next_locations(l.cbegin(), l.cend());
		auto rng = std::default_random_engine{};
		std::shuffle(std::begin(next_locations), std::end(next_locations), rng);
		for (int next : next_locations)
		{
			if (validMove(curr, next))
			{
				curr = next;
				break;
			}
		}
	}
	return curr;
}

void Instance::generateRandomAgents(int warehouse_width)
{
	cout << "Generate " << num_of_agents << " random start and goal locations " << endl;
	vector<bool> starts(map_size, false);
	vector<bool> goals(map_size, false);
	start_locations.resize(num_of_agents);
	goal_locations.resize(num_of_agents);

	if (warehouse_width == 0)//Generate agents randomly
	{
		// Choose random start locations
		int k = 0;
		while ( k < num_of_agents)
		{
			int x = rand() % num_of_rows, y = rand() % num_of_cols;
			int start = linearizeCoordinate(x, y);
			if (my_map[start] || starts[start])
				continue;

			// find goal
            x = rand() % num_of_rows;
            y = rand() % num_of_cols;
            int goal = linearizeCoordinate(x, y);
            while (my_map[goal] || goals[goal])
            {
                x = rand() % num_of_rows;
                y = rand() % num_of_cols;
                goal = linearizeCoordinate(x, y);
            }
            if (!isConnected(start, goal))
                continue;

			/*int goal = randomWalk(start, RANDOM_WALK_STEPS);
			while (goals[goal])
				goal = randomWalk(goal, 1);*/

            // update start
            start_locations[k] = start;
            starts[start] = true;
			//update goal
			goal_locations[k] = goal;
			goals[goal] = true;

			k++;
		}
	}
	else //Generate agents for warehouse scenario
	{
		// Choose random start locations
		int k = 0;
		while (k < num_of_agents)
		{
			int x = rand() % num_of_rows, y = rand() % warehouse_width;
			if (k % 2 == 0)
				y = num_of_cols - y - 1;
			int start = linearizeCoordinate(x, y);
			if (starts[start])
				continue;
			// update start
			start_locations[k] = start;
			starts[start] = true;

			k++;
		}
		// Choose random goal locations
		k = 0;
		while (k < num_of_agents)
		{
			int x = rand() % num_of_rows, y = rand() % warehouse_width;
			if (k % 2 == 1)
				y = num_of_cols - y - 1;
			int goal = linearizeCoordinate(x, y);
			if (goals[goal])
				continue;
			// update goal
			goal_locations[k] = goal;
			goals[goal] = true;
			k++;
		}
	}
}


bool Instance::addObstacle(int obstacle)
{
	if (my_map[obstacle])
		return false;
	my_map[obstacle] = true;
	int obstacle_x = getRowCoordinate(obstacle);
	int obstacle_y = getColCoordinate(obstacle);
	int x[4] = { obstacle_x, obstacle_x + 1, obstacle_x, obstacle_x - 1 };
	int y[4] = { obstacle_y - 1, obstacle_y, obstacle_y + 1, obstacle_y };
	int start = 0;
	int goal = 1;
	while (start < 3 && goal < 4)
	{
		if (x[start] < 0 || x[start] >= num_of_rows || y[start] < 0 || y[start] >= num_of_cols 
			|| my_map[linearizeCoordinate(x[start], y[start])])
			start++;
		else if (goal <= start)
			goal = start + 1;
		else if (x[goal] < 0 || x[goal] >= num_of_rows || y[goal] < 0 || y[goal] >= num_of_cols 
			|| my_map[linearizeCoordinate(x[goal], y[goal])])
			goal++;
		else if (isConnected(linearizeCoordinate(x[start], y[start]), linearizeCoordinate(x[goal], y[goal]))) // cannot find a path from start to goal 
		{
			start = goal;
			goal++;
		}
		else
		{
			my_map[obstacle] = false;
			return false;
		}
	}
	return true;
}

bool Instance::isConnected(int start, int goal)
{
	std::queue<int> open;
	vector<bool> closed(map_size, false);
	open.push(start);
	closed[start] = true;
	while (!open.empty())
	{
		int curr = open.front(); open.pop();
		if (curr == goal)
			return true;
		for (int next : getNeighbors(curr))
		{
			if (closed[next])
				continue;
			open.push(next);
			closed[next] = true;
		}
	}
	return false;
}

void Instance::generateConnectedRandomGrid(int rows, int cols, int obstacles)
{
	cout << "Generate a " << rows << " x " << cols << " grid with " << obstacles << " obstacles. " << endl;
	int i, j;
	num_of_rows = rows + 2;
	num_of_cols = cols + 2;
	map_size = num_of_rows * num_of_cols;
	my_map.resize(map_size, false);
	// Possible moves [WAIT, NORTH, EAST, SOUTH, WEST]
	/*moves_offset[Instance::valid_moves_t::WAIT_MOVE] = 0;
	moves_offset[Instance::valid_moves_t::NORTH] = -num_of_cols;
	moves_offset[Instance::valid_moves_t::EAST] = 1;
	moves_offset[Instance::valid_moves_t::SOUTH] = num_of_cols;
	moves_offset[Instance::valid_moves_t::WEST] = -1;*/

	// add padding
	i = 0;
	for (j = 0; j<num_of_cols; j++)
		my_map[linearizeCoordinate(i, j)] = true;
	i = num_of_rows - 1;
	for (j = 0; j<num_of_cols; j++)
		my_map[linearizeCoordinate(i, j)] = true;
	j = 0;
	for (i = 0; i<num_of_rows; i++)
		my_map[linearizeCoordinate(i, j)] = true;
	j = num_of_cols - 1;
	for (i = 0; i<num_of_rows; i++)
		my_map[linearizeCoordinate(i, j)] = true;

	// add obstacles uniformly at random
	i = 0;
	while (i < obstacles)
	{
		int loc = rand() % map_size;
		if (addObstacle(loc))
		{
			printMap();
			i++;
		}
	}
}

bool Instance::loadMap()
{
	using namespace boost;
	using namespace std;
	ifstream myfile(map_fname.c_str());
	if (!myfile.is_open())
		return false;
	string line;
	tokenizer< char_separator<char> >::iterator beg;
	getline(myfile, line);
	if (line[0] == 't') // Nathan's benchmark
	{
        nathan_benchmark = true;
		char_separator<char> sep(" ");
		getline(myfile, line);
		tokenizer< char_separator<char> > tok(line, sep);
		beg = tok.begin();
		beg++;
		num_of_rows = atoi((*beg).c_str()); // read number of rows
		getline(myfile, line);
		tokenizer< char_separator<char> > tok2(line, sep);
		beg = tok2.begin();
		beg++;
		num_of_cols = atoi((*beg).c_str()); // read number of cols
		getline(myfile, line); // skip "map"
	}
	else // my benchmark
	{
        nathan_benchmark = false;
		char_separator<char> sep(",");
		tokenizer< char_separator<char> > tok(line, sep);
		beg = tok.begin();
		num_of_rows = atoi((*beg).c_str()); // read number of rows
		beg++;
		num_of_cols = atoi((*beg).c_str()); // read number of cols
	}
	map_size = num_of_cols * num_of_rows;
	my_map.resize(map_size, false);
	// read map (and start/goal locations)
	for (int i = 0; i < num_of_rows; i++) {
		getline(myfile, line);
		for (int j = 0; j < num_of_cols; j++) {
			my_map[linearizeCoordinate(i, j)] = (line[j] != '.');
		}
	}
	myfile.close();

	// initialize moves_offset array
	/*moves_offset[Instance::valid_moves_t::WAIT_MOVE] = 0;
	moves_offset[Instance::valid_moves_t::NORTH] = -num_of_cols;
	moves_offset[Instance::valid_moves_t::EAST] = 1;
	moves_offset[Instance::valid_moves_t::SOUTH] = num_of_cols;
	moves_offset[Instance::valid_moves_t::WEST] = -1;*/
	return true;
}


void Instance::printMap() const
{
	for (int i = 0; i< num_of_rows; i++)
	{
		for (int j = 0; j < num_of_cols; j++)
		{
			if (this->my_map[linearizeCoordinate(i, j)])
				cout << '@';
			else
				cout << '.';
		}
		cout << endl;
	}
}


void Instance::saveMap() const
{
	ofstream myfile;
	myfile.open(map_fname);
	if (!myfile.is_open())
	{
		cout << "Fail to save the map to " << map_fname << endl;
		return;
	}
	myfile << num_of_rows << "," << num_of_cols << endl;
	for (int i = 0; i < num_of_rows; i++)
	{
		for (int j = 0; j < num_of_cols; j++)
		{
			if (my_map[linearizeCoordinate(i, j)])
				myfile << "@";
			else
				myfile << ".";
		}
		myfile << endl;
	}
	myfile.close();
}


bool Instance::loadAgents()
{
	using namespace std;
	using namespace boost;

	string line;
	ifstream myfile (agent_fname.c_str());
	if (!myfile.is_open()) 
	return false;

	getline(myfile, line);
	if (nathan_benchmark) // Nathan's benchmark
	{
		if (num_of_agents == 0)
		{
			cerr << "The number of agents should be larger than 0" << endl;
			exit(-1);
		}
		start_locations.resize(num_of_agents);
		goal_locations.resize(num_of_agents);
		char_separator<char> sep("\t");
		for (int i = 0; i < num_of_agents; i++)
		{
			getline(myfile, line);
			if (line.empty())
            {
			    cerr << "Error! The instance has only " << i << " agents" << endl;
			    exit(-1);
            }
			tokenizer< char_separator<char> > tok(line, sep);
			tokenizer< char_separator<char> >::iterator beg = tok.begin();
			beg++; // skip the first number
			beg++; // skip the map name
			beg++; // skip the columns
			beg++; // skip the rows
				   // read start [row,col] for agent i
			int col = atoi((*beg).c_str());
			beg++;
			int row = atoi((*beg).c_str());
			start_locations[i] = linearizeCoordinate(row, col);
			// read goal [row,col] for agent i
			beg++;
			col = atoi((*beg).c_str());
			beg++;
			row = atoi((*beg).c_str());
			goal_locations[i] = linearizeCoordinate(row, col);
		}
	}
	else // My benchmark
	{
		char_separator<char> sep(",");
		tokenizer< char_separator<char> > tok(line, sep);
		tokenizer< char_separator<char> >::iterator beg = tok.begin();
		num_of_agents = atoi((*beg).c_str());
		start_locations.resize(num_of_agents);
		goal_locations.resize(num_of_agents);
		for (int i = 0; i<num_of_agents; i++)
		{
			getline(myfile, line);
			tokenizer< char_separator<char> > col_tok(line, sep);
			tokenizer< char_separator<char> >::iterator c_beg = col_tok.begin();
			pair<int, int> curr_pair;
			// read start [row,col] for agent i
			int row = atoi((*c_beg).c_str());
			c_beg++;
			int col = atoi((*c_beg).c_str());
			start_locations[i] = linearizeCoordinate(row, col);
			// read goal [row,col] for agent i
			c_beg++;
			row = atoi((*c_beg).c_str());
			c_beg++;
			col = atoi((*c_beg).c_str());
			goal_locations[i] = linearizeCoordinate(row, col);
		}
	}
	myfile.close();
	return true;

}


void Instance::printAgents() const
{
  for (int i = 0; i < num_of_agents; i++) 
  {
    cout << "Agent" << i << " : S=(" << getRowCoordinate(start_locations[i]) << "," << getColCoordinate(start_locations[i]) 
				<< ") ; G=(" << getRowCoordinate(goal_locations[i]) << "," << getColCoordinate(goal_locations[i]) << ")" << endl;
  }
}


void Instance::saveAgents() const
{
  ofstream myfile;
  myfile.open(agent_fname);
  if (!myfile.is_open())
  {
	  cout << "Fail to save the agents to " << agent_fname << endl;
	  return;
  }
  myfile << num_of_agents << endl;
  for (int i = 0; i < num_of_agents; i++)
    myfile << getRowCoordinate(start_locations[i]) << "," << getColCoordinate(start_locations[i]) << ","
           << getRowCoordinate(goal_locations[i]) << "," << getColCoordinate(goal_locations[i]) << "," << endl;
  myfile.close();
}

void Instance::saveNathan() const
{
    ofstream myfile;
    myfile.open(agent_fname); // +"_nathan.scen");
    if (!myfile.is_open())
    {
        cout << "Fail to save the agents to " << agent_fname << endl;
        return;
    }
    myfile << "version 1" << endl;
    for (int i = 0; i < num_of_agents; i++)
        myfile << i<<"\t"<<map_fname<<"\t"<<this->num_of_cols<<"\t"<<this->num_of_rows<<"\t"
                << getColCoordinate(start_locations[i]) << "\t" << getRowCoordinate(start_locations[i]) << "\t"
                << getColCoordinate(goal_locations[i]) << "\t" << getRowCoordinate(goal_locations[i]) << "\t"  <<0<< endl;
    myfile.close();
}


list<int> Instance::getNeighbors(int curr) const
{
	list<int> neighbors;
	int candidates[4] = {curr + 1, curr - 1, curr + num_of_cols, curr - num_of_cols};
	for (int next : candidates)
	{
		if (validMove(curr, next))
			neighbors.emplace_back(next);
	}
	return neighbors;
}

void Instance::savePaths(const string & file_name, const vector<Path*>& paths) const
{
    std::ofstream output;
    output.open(file_name);

    for (auto i = 0; i < paths.size(); i++)
    {
        output << "Agent " << i << ":";
        for (const auto &state : (*paths[i]))
        {
            if (nathan_benchmark)
                output << "(" << getColCoordinate(state.location) << "," << getRowCoordinate(state.location) << ")->";
            else
                output << "(" << getRowCoordinate(state.location) << "," << getColCoordinate(state.location) << ")->";
        }
        output << endl;
    }
    output.close();
}

bool Instance::validateSolution(const vector<Path*>& paths, int sum_of_costs, int num_of_colliding_pairs) const
{
    cout << "Validate solution ..." << endl;
    if (paths.size() != start_locations.size())
    {
        cerr << "We have " << paths.size() << " for " << start_locations.size() << " agents." << endl;
        exit(-1);
    }
    int sum = 0;
    for (auto i = 0; i < start_locations.size(); i++)
    {
        if (paths[i] == nullptr or paths[i]->empty())
        {
            cerr << "No path for agent " << i << endl;
            exit(-1);
        }
        else if (start_locations[i] != paths[i]->front().location)
        {
            cerr << "The path of agent " << i << " starts from location " << paths[i]->front().location
                 << ", which is different from its start location " << start_locations[i] << endl;
            exit(-1);
        }
        else if (goal_locations[i] != paths[i]->back().location)
        {
            cerr << "The path of agent " << i << " ends at location " << paths[i]->back().location
                 << ", which is different from its goal location " << goal_locations[i] << endl;
            exit(-1);
        }
        for (int t = 1; t < (int) paths[i]->size(); t++ )
        {
            if (!validMove(paths[i]->at(t - 1).location, paths[i]->at(t).location))
            {
                cerr << "The path of agent " << i << " jumps from "
                     << paths[i]->at(t - 1).location << " to " << paths[i]->at(t).location
                     << " between timesteps " << t - 1 << " and " << t << endl;
                exit(-1);
            }
        }
        sum += (int) paths[i]->size() - 1;
    }
    if (sum_of_costs != sum)
    {
        cerr << "The computed sum of costs " << sum_of_costs <<
             " is different from that of the solution " << sum << endl;
        exit(-1);
    }
    // check for colliions
    int collisions = 0;
    for (auto i = 0; i < start_locations.size(); i++)
    {
        for (auto j = i + 1; j < start_locations.size(); j++)
        {
            bool found_collision = false;
            const auto a1 = paths[i]->size() <= paths[j]->size()? i : j;
            const auto a2 = paths[i]->size() <= paths[j]->size()? j : i;
            int t = 1;
            for (; t < (int) paths[a1]->size(); t++)
            {
                if (paths[a1]->at(t).location == paths[a2]->at(t).location) // vertex conflict
                {
                    if (num_of_colliding_pairs == 0)
                    {
                        cerr << "Find a vertex conflict between agents " << a1 << " and " << a2 <<
                             " at location " << paths[a1]->at(t).location << " at timestep " << t << endl;
                        exit(-1);
                    }
                    collisions++;
                    found_collision = true;
                    break;
                }
                else if (paths[a1]->at(t).location == paths[a2]->at(t-1).location &&
                        paths[a1]->at(t-1).location == paths[a2]->at(t).location) // edge conflict
                {
                    if (num_of_colliding_pairs == 0)
                    {
                        cerr << "Find an edge conflict between agents " << a1 << " and " << a2 <<
                             " at edge (" << paths[a1]->at(t-1).location << "," << paths[a1]->at(t).location <<
                             ") at timestep " << t << endl;
                        exit(-1);
                    }
                    collisions++;
                    found_collision = true;
                    break;
                }
            }
            if (!found_collision)
            {
                auto target = paths[a1]->back().location;
                for (; t < (int) paths[a2]->size(); t++)
                {
                    if (paths[a2]->at(t).location == target)  // target conflict
                    {
                        if (num_of_colliding_pairs == 0)
                        {
                            cerr << "Find a target conflict where agent " << a2 << " (of length " << paths[a2]->size() - 1 <<
                                 ") traverses agent " << a1 << " (of length " << paths[a1]->size() - 1<<
                                 ")'s target location " << target << " at timestep " << t << endl;
                            exit(-1);
                        }
                        collisions++;
                        break;
                    }
                }
            }
        }
    }
    if (collisions != num_of_colliding_pairs)
    {
        cerr << "The computed number of colliding pairs " << num_of_colliding_pairs <<
             " is different from that of the solution " << collisions << endl;
        exit(-1);
    }
    cout << "Done!" << endl;
    return true;
}
} // end namespace LNS
