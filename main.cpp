#include "bits/stdc++.h"
using namespace std;
#define mp make_pair
// oh god
#define this it
#include <bc.h>
#undef this

bool check_errors() 
{
    if (bc_has_err()) 
    {
        char *err;
        /// Note: this clears the current global error.
        int8_t code = bc_get_last_err(&err);
        printf("Engine error code %d: %s\n", code, err);
        bc_free_string(err);
        return true;
    } 
    else 
    {
        return false;
    }
};
struct MarsSTRUCT //contains a more readable map of mars, as well as the code to find where to send a rocket
{
    int r, c, numberOfWorkers;
    int mars[60][60]; // 0 = passable, 1 = unpassable
    int karbonite[60][60]; // number of karbonite at each square at some time
    int seen[60][60], upto, dis[60][60], comp[60][60], compsize[3000], hasrocket[60][60], robotthatlead[60][60], firstdir[60][60], hasUnit[60][60];
    int rocketsInComp[3000], mnInAComp, workersInComp[3000];
    set<pair<int, int> > taken;
    unordered_set<int> landedRockets;
    bc_AsteroidPattern* asteroidPattern;
    void init(bc_GameController* gc)
    {
        bc_Planet bc_Planet_mars = Mars;
        bc_PlanetMap* map = bc_GameController_starting_map(gc, bc_Planet_mars);
        r = bc_PlanetMap_height_get(map);
        c = bc_PlanetMap_width_get(map);
        bc_MapLocation* loc = new_bc_MapLocation(bc_Planet_mars, 0, 0);
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {
                bc_MapLocation_x_set(loc, i);
                bc_MapLocation_y_set(loc, j);
                mars[i][j] = !bc_PlanetMap_is_passable_terrain_at(map, loc);
            }
        }
        delete_bc_PlanetMap(map);

        // Add the first 100 rounds of asteriod strikes to the karbonite array
        asteroidPattern = bc_GameMap_asteroids_get(new_bc_GameMap());
        bc_AsteroidStrike* asteroidStrike;
        for (int i = 1; i <= 100; i++)
        {
            if (bc_AsteroidPattern_has_asteroid(asteroidPattern, i))
            {
                asteroidStrike = bc_AsteroidPattern_asteroid(asteroidPattern, i);
                loc = bc_AsteroidStrike_location_get(asteroidStrike);
                int x = bc_MapLocation_x_get(loc);
                int y = bc_MapLocation_y_get(loc);
                printf("Asteroid strike! %d %d\n", x, y);
                delete_bc_MapLocation(loc);
                delete_bc_AsteroidStrike(asteroidStrike);
            }
        }

    }
    void findcomps(int x, int y, int k) // find components and their sizes
    {
        if (seen[x][y] == upto) return;
        seen[x][y] = upto;
        comp[x][y] = k;
        compsize[k]++;
        for (int i = max(x-1, 0); i <= min(x+1, c-1); i++)
        {
            for (int j = max(y-1, 0); j <= min(y+1, r-1); j++)
            {
                if (!mars[i][j]) 
                { 
                	findcomps(i, j, k);
                }
            }
        }
    }
    int bfs(int x, int y, int k) // bfs for finding amount of karbonite within distance k
    {
        int ans = 0;
        pair<int, int> q[3000];
        int s = 0;
        int e = 0;
        q[e++] = make_pair(s, e);
        seen[x][y] = ++upto;
        dis[x][y] = 0;
        while (s != e)
        {
            int x = q[s].first;
            int y = q[s].second;
            s++;
            int d = dis[x][y];
            ans += karbonite[x][y];
            if (d == k || !mars[x][y]) continue;
            for (int i = max(x-1, 0); i <= min(x+1, c-1); i++)
            {
                for (int j = max(y-1, 0); j <= min(y+1, r-1); j++)
                {
                    if (seen[i][j] != upto)
                    {
                        seen[i][j] = upto;
                        dis[i][j] = d+1;
                        q[e++] = make_pair(s, e);
                    }
                }
            }
        }
        return ans;
    }
    int numneighbours(int x, int y) // returns number of valid neighbours
    {
        int ans = -1;
        for (int i = max(x-1, 0); i <= min(x+1, c-1); i++)
        {
            for (int j = max(y-1, 0); j <= min(y+1, r-1); j++)
            {
                if (!mars[i][j]) ans++;
            }
        }
        return ans;
    }

    pair<int, int> optimalsquare(bool count = true)
    {
        int A = 1; //constant, representing how much weighting the amount of karbonite should have
        int B = 1; //weighting of size of component
        int C = 1; //weighting of amount of neighbours
        // A (and possibly also B/C) could be dependent on total available karbonite/average comp size/whatever
        int K = 5; //dist to consider for karbonite
        upto++;
        mnInAComp = 999999999;
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++) 
            { 
                if (!mars[i][j]) 
                {
                	findcomps(i, j, i*c+j);
                	mnInAComp = min(mnInAComp, rocketsInComp[i*c+j]);
                }
            }
        }
        pair<pair<int, int>, pair<int, int> > best = mp(mp(0, 0), mp(0, 0));
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {   
                if (mars[i][j] || hasrocket[i][j] || rocketsInComp[comp[i][j]] != mnInAComp) continue;
                int nearbykar = bfs(i, j, K);
                int compsz = compsize[comp[i][j]];
                int numnei = numneighbours(i, j);
                int rating = A*nearbykar + B*compsz + C*numnei;
                best = max(best, mp(mp(rating, rand()), mp(i, j))); //rand is so that it picks randomly among ties, rather than picking highest x and y
            }
        }
        if (count) hasrocket[best.second.first][best.second.second] = true;
        if (count) rocketsInComp[comp[best.second.first][best.second.second]]++;
        return best.second;
    }   
    void updateKarboniteAmount(bc_GameController* gc) // this function will be used until the asteroid thing is fixed
    { // I think this only has karbonite within the viewable range ......... rip.....
        bc_MapLocation* loc;
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {
                loc = new_bc_MapLocation(Mars, i, j);
                karbonite[i][j] = bc_GameController_karbonite_at(gc, loc);
            //    if (karbonite[i][j]) printf("at %d %d\n", i, j);
         	 //   if (bc_GameController_has_unit_at_location(gc, loc)) { hasUnit[i][j] = 1; printf("helllo\n"); }
            //    else hasUnit[i][j] = 0;
                hasUnit[i][j] = 0;
                delete_bc_MapLocation(loc);
               	workersInComp[comp[i][j]] = 0;
            }
        }
    }
};
struct EarthSTRUCT //contains a more readable map of earth
{
    int r, c;
    int earth[60][60]; // 0 = passable, 1 = unpassable
    int karbonite[60][60]; // number of karbonite at each square at some time
    void init(bc_GameController* gc)
    {
        bc_Planet bc_Planet_earth = Earth;
        bc_PlanetMap* map = bc_GameController_starting_map(gc, bc_Planet_earth);
        r = bc_PlanetMap_height_get(map);
        c = bc_PlanetMap_width_get(map);
        bc_MapLocation* loc = new_bc_MapLocation(bc_Planet_earth, 0, 0);
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {
                bc_MapLocation_x_set(loc, i);
                bc_MapLocation_y_set(loc, j);
                earth[i][j] = !bc_PlanetMap_is_passable_terrain_at(map, loc);
               // printf("%d ", earth[i][j]);
            }
          //  printf("\n");
        }
        delete_bc_MapLocation(loc);
        delete_bc_PlanetMap(map);
    }
};

MarsSTRUCT mars;
EarthSTRUCT earth;

bool enemyFactory[60][60], enemyRocket[60][60];
// which rocket / factory a unit is assigned to
unordered_map<uint16_t, uint16_t> assignedStructure;
// which directions are assigned for a structure
unordered_map<uint16_t, int> dirAssigned;
// how many workers a rocket / factory should have
unordered_map<uint16_t, int> reqAssignees;

// returns true if successful
bool createBlueprint(bc_GameController* gc, bc_Unit* mainWorker, uint16_t id, int num, bc_Direction dir, bc_UnitType type) {
    if (type != Factory && type != Rocket)
    {
        printf("Tried to blueprint invalid unit type.\n");
        return false;
    }

    bc_Location *loc = bc_Unit_location(mainWorker);
    bc_MapLocation *mapLoc = bc_Location_map_location(loc);
    bc_MapLocation *newLoc = bc_MapLocation_add(mapLoc, dir);

    printf("blueprinting at: %d %d\n", bc_MapLocation_x_get(newLoc), bc_MapLocation_y_get(newLoc));

    // TODO: This is a really really really bad workaround
    // because has_unit_at_location is rip.
    bc_Unit *tmp = bc_GameController_sense_unit_at_location(gc, newLoc);
    if (tmp)
    {
        printf("Tried to build on a unit.\n");

        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
        delete_bc_MapLocation(newLoc);
        delete_bc_Unit(tmp);
        return false;
    }

    if (bc_GameController_can_blueprint(gc, id, type, dir) &&
        bc_UnitType_blueprint_cost(type) <= bc_GameController_karbonite(gc))
    {
        printf("Blueprinting...\n");
        bc_GameController_blueprint(gc, id, type, dir);

        // figure out the id of the newly blueprinted factory / rocket
        bc_Unit *structure = bc_GameController_sense_unit_at_location(gc, newLoc);
        uint16_t structureid = bc_Unit_id(structure);

        assignedStructure[id] = structureid;
        reqAssignees[structureid] = num;

        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
        delete_bc_MapLocation(newLoc);
        delete_bc_Unit(structure);
        return true;
    }

    printf("Failed to blueprint\n");

    delete_bc_Location(loc);
    delete_bc_MapLocation(mapLoc);
    delete_bc_MapLocation(newLoc);
    return false;
}

void mineKarboniteOnMars(bc_GameController* gc) // Controls the mining of Karbonite on mars
{
    mars.taken.clear();
    mars.updateKarboniteAmount(gc);
    bc_VecUnit *units = bc_GameController_my_units(gc); 
    int len = bc_VecUnit_len(units);
    vector<bc_Unit*> canMove;
    for (int l = 0; l < len; l++) 
    {
        bc_Unit *unit = bc_VecUnit_index(units, l);
        bc_UnitType unitType = bc_Unit_unit_type(unit);
        uint16_t id = bc_Unit_id(unit);
        bc_Location* loc = bc_Unit_location(unit);
        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
        int x = bc_MapLocation_x_get(mapLoc);
        int y = bc_MapLocation_y_get(mapLoc);
        mars.hasUnit[x][y] = 1;
        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
        if (unitType == Worker)
        {
            for (int i = 8; i >= 0; i--)
            {
                if (bc_GameController_can_harvest(gc, id, (bc_Direction)i))
                {
                    printf("harvesting\n");
                    bc_GameController_harvest(gc, id, (bc_Direction)i); // harvest the karbonite
                    break; // we can only harvest 1 per turn :(
                }
            }
            if (true || bc_GameController_is_move_ready(gc, id)) canMove.push_back(unit); // consider all workers, rather than the ones that can move
            else delete_bc_Unit(unit);
           	mars.workersInComp[mars.comp[x][y]]++;
        }
        else if (unitType == Rocket) // unload
        {
            for (int i = 0; i < 8; i++)
            {
                if (bc_GameController_can_unload(gc, id, (bc_Direction)i)) // try unloading in all directions
                {
                    printf("Unloading\n");
                    bc_GameController_unload(gc, id, (bc_Direction)i);
                    mars.numberOfWorkers++; // Currently the code assumes all things in rockets are workers.
                }
            }
            if (mars.landedRockets.find(id) == mars.landedRockets.end())
            {
            	mars.landedRockets.insert(id);
            	mars.rocketsInComp[mars.comp[x][y]]++;
            }
         /*   bc_VecUnitID *garrisonUnits = bc_Unit_structure_garrison(unit);
            int len = bc_VecUnitID_len(garrisonUnits);
            delete_bc_VecUnitID(garrisonUnits);
            if (!len)
            {
                printf("Trying to disintegrate\n");
                bc_GameController_disintegrate_unit(gc, id);
            }
            else printf("%d\n", len);
            delete_bc_Unit(unit);*/
        }   
    }
    for (auto unit : canMove)
    {
    	bc_Location* loc = bc_Unit_location(unit);
        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
        uint16_t id = bc_Unit_id(unit);
        int x = bc_MapLocation_x_get(mapLoc);
        int y = bc_MapLocation_y_get(mapLoc);
        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
        if (mars.workersInComp[mars.comp[x][y]] < 2*mars.rocketsInComp[mars.comp[x][y]]) // we want to have 2 workers per rocket that landed
        {
        	for (int i = 0; i < 8; i++)
        	{
        		if (bc_GameController_can_replicate(gc, id, (bc_Direction)i))
        		{
        			printf("Duplicated worker\n");
        			bc_GameController_replicate(gc, id, (bc_Direction)i);
        			mars.workersInComp[mars.comp[x][y]]++;
        			break;
        		}
        	}
        }
    }
    while (!canMove.empty()) // where should we move to
    {
        bool worked = false;
        mars.upto++;
        queue<pair<int, int> > q;
        for (int i = 0; i < canMove.size(); i++)
        {
            auto unit = canMove[i];
            bc_Location* loc = bc_Unit_location(unit);
            bc_MapLocation* mapLoc = bc_Location_map_location(loc);
            int x = bc_MapLocation_x_get(mapLoc);
            int y = bc_MapLocation_y_get(mapLoc);
            mars.seen[x][y] = mars.upto;
            mars.robotthatlead[x][y] = i;
            mars.firstdir[x][y] = 8;
            q.emplace(x, y);
            delete_bc_MapLocation(mapLoc);
            delete_bc_Location(loc);
        }
        while (!q.empty())
        {
            int x = q.front().first;
            int y = q.front().second;
          //  if (x == 12 && y == 10) printf("YAYAYAY\n");
         //   printf("%d %d\n", x, y);
            q.pop();
            if (mars.karbonite[x][y] && mars.taken.find(make_pair(x, y)) == mars.taken.end())
            {
                // We've reached some karbonite. Send the robot in question here;
                mars.taken.insert(make_pair(x, y));

                bc_Unit* unit = canMove[mars.robotthatlead[x][y]];
                canMove.erase(canMove.begin()+mars.robotthatlead[x][y]);
                int dir = mars.firstdir[x][y];
                uint16_t id = bc_Unit_id(unit);
                // Update the location of robots;
                bc_Location* loc = bc_Unit_location(unit);
                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                int i = bc_MapLocation_x_get(mapLoc);
                int j = bc_MapLocation_y_get(mapLoc);
                delete_bc_MapLocation(mapLoc);
                delete_bc_Location(loc);
                mars.hasUnit[i][j] = 0;
                if (bc_GameController_can_move(gc, id, (bc_Direction)dir))
                {
                    bc_GameController_move_robot(gc, id, (bc_Direction)dir);
                }
                // Set the newloc
                loc = bc_Unit_location(unit);
                mapLoc = bc_Location_map_location(loc);
                i = bc_MapLocation_x_get(mapLoc);
                j = bc_MapLocation_y_get(mapLoc);
                delete_bc_MapLocation(mapLoc);
                delete_bc_Location(loc);
                mars.hasUnit[i][j] = 1;
                delete_bc_Unit(unit);
                worked = true;
                break;
            }   
            if (mars.mars[x][y] || (mars.hasUnit[x][y] && mars.firstdir[x][y] != 8)) continue;
            for (int l = 0; l < 8; l++) // consider going this direction
            {
                int i = x + bc_Direction_dx((bc_Direction)l);
                int j = y + bc_Direction_dy((bc_Direction)l);
                if (i >= 0 && i < mars.c && j >= 0 && j < mars.r && mars.seen[i][j] != mars.upto)
                {
                    mars.seen[i][j] = mars.upto;
                    mars.robotthatlead[i][j] = mars.robotthatlead[x][y];
                    if (mars.firstdir[x][y] == 8) mars.firstdir[i][j] = l;
                    else mars.firstdir[i][j] = mars.firstdir[x][y];
                    q.emplace(i, j);
                }
            }
        }
        if (!worked)
        {
        //    printf("rip...\n");
            for (auto unit : canMove)
            {
                delete_bc_Unit(unit);
            }
            canMove.clear();
        }
    }
}

// returns: [the nearest location, the direction to go to get to the enemy]
pair<bc_MapLocation*, bc_Direction> findNearestEnemy(bc_GameController* gc, bc_Team currTeam, bc_PlanetMap* currPlanet,
            bc_MapLocation* mapLoc, int range, bool targetByType, bc_UnitType targetType = Worker) {
    bool seenLoc[60][60];
    bc_Direction prev[60][60];
    for (int i = 0; i < 60; ++i) for (int j = 0; j < 60; ++j)
    {
        seenLoc[i][j] = 0;
        prev[i][j] = Center;
    }

    int ox = bc_MapLocation_x_get(mapLoc), oy = bc_MapLocation_y_get(mapLoc);
    seenLoc[ox][oy] = 1;
    queue<bc_MapLocation*> Q;

    //printf("Searching for nearby enemies...\n");

    Q.push(bc_MapLocation_clone(mapLoc));
    while (Q.size())
    {
        bc_MapLocation* currLoc = Q.front(); Q.pop();

        if (!bc_PlanetMap_is_passable_terrain_at(currPlanet, currLoc) ||
            !bc_MapLocation_is_within_range(currLoc, range, mapLoc))
        {
            delete_bc_MapLocation(currLoc);

            continue;
        }

        int x = bc_MapLocation_x_get(currLoc), y = bc_MapLocation_y_get(currLoc);

        for (int i = 0; i < 8; ++i)
        {
            bc_Direction dir = (bc_Direction)i;
            int nx = x + bc_Direction_dx(dir), ny = y + bc_Direction_dy(dir);

            if (nx < 0 || nx >= bc_PlanetMap_width_get(currPlanet) ||
                ny < 0 || ny >= bc_PlanetMap_height_get(currPlanet) ||
                seenLoc[nx][ny])
            {
                continue;
            }

            seenLoc[nx][ny] = 1;
            prev[nx][ny] = dir;

            Q.push(bc_MapLocation_add(currLoc, dir));

            // DON'T DELETE IT HERE:
            // We need it later in the BFS
        }

        // TODO: another thing for
        // has_unit_at_location... maybe?
        bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, currLoc);
        if (!unit)
        {
            delete_bc_MapLocation(currLoc);

            continue;
        }

        bc_Team team = bc_Unit_team(unit);
        bc_UnitType type = bc_Unit_unit_type(unit);

        if (team != currTeam && (!targetByType || type == targetType)) 
        {
            delete_bc_Unit(unit);

            // Walk backwards in the prev array
            // to determine the optimal move from
            // mapLoc.
            bc_Direction dir;
            while (x ^ ox || y ^ oy) {
                dir = prev[x][y];
                // subtract - take the opposite direction
                x -= bc_Direction_dx(dir);
                y -= bc_Direction_dy(dir);
            }

            // Get rid of all other locations - prevent mem leaks
            while (Q.size()) {
                bc_MapLocation* cml = Q.front(); Q.pop();
                delete_bc_MapLocation(cml);
            }

            return {currLoc, dir};
        }

        delete_bc_MapLocation(currLoc);
        delete_bc_Unit(unit);
    }

    return {0, Center};
}

struct RangerStrat
{
	int firstdir[60][60];
	int r, c, seen[60][60], upto, dis[60][60];
	vector<pair<int, int> > atDis[5010];
	bc_Planet myPlanet;
	bc_Team myTeam;
	bc_GameController* gc;
	void init(bc_GameController* GC, bc_Planet planet, bc_Team team)
	{
		gc = GC;
		myPlanet = planet;
        myTeam = team;
        bc_PlanetMap* map = bc_GameController_starting_map(gc, planet);
        r = bc_PlanetMap_height_get(map);
        c = bc_PlanetMap_width_get(map);
        delete_bc_PlanetMap(map);
	}
	bool friendly(int x, int y)
	{
		bc_MapLocation* loc = new_bc_MapLocation(myPlanet, x, y);
		if (bc_GameController_has_unit_at_location(gc, loc))
		{
			bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, loc);
			if (bc_Unit_team(unit) == myTeam)
			{
				delete_bc_MapLocation(loc);
				delete_bc_Unit(unit);
				return true;
			}
			delete_bc_Unit(unit);
		}
		delete_bc_MapLocation(loc);
		return false;
	}
	bool enemy(int x, int y)
	{
		bc_MapLocation* loc = new_bc_MapLocation(myPlanet, x, y);
		if (bc_GameController_has_unit_at_location(gc, loc))
		{
			bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, loc);
			if (bc_Unit_team(unit) != myTeam)
			{
				delete_bc_MapLocation(loc);
				delete_bc_Unit(unit);
				return true;
			}
			delete_bc_Unit(unit);
		}
		delete_bc_MapLocation(loc);
		return false;
	}
	bool existsOnMap(int x, int y)
	{
		if (friendly(x, y)) return false;
		if (x < 0 || y < 0 || x >= c || y >= r) return false;
		if (myPlanet == Earth) return !earth.earth[x][y];
		else return !mars.mars[x][y];
	}
	// for rangers: does all their work
	// also rip indenting .... 
	void findNearestEnemy(bc_GameController* gc, bc_Unit* unit)
	{	
		// so we want to consider the 9 possible spots we could move to (including our current location)
		// each of them mark their nearest enemy within 2 moves (if such an enemy exist). If one does out square is *not* safe
		// Then just go to the safest square and attack an enemy. Seems good
		//printf("starting\n");
		uint16_t id = bc_Unit_id(unit);
		bc_Location* loc = bc_Unit_location(unit);
		if (!bc_Location_is_on_planet(loc, Mars) && !bc_Location_is_on_planet(loc, Earth)) return;
		bc_MapLocation* mapLoc = bc_Location_map_location(loc);
		int x = bc_MapLocation_x_get(mapLoc), y = bc_MapLocation_y_get(mapLoc);
		delete_bc_Location(loc);
		delete_bc_MapLocation(mapLoc);
		vector<int> good;
		for (int l = 0; l <= 8; l++)
		{
			int i = x + bc_Direction_dx((bc_Direction)l);
    	    int j = y + bc_Direction_dy((bc_Direction)l);
        	if (!existsOnMap(i, j)) continue;
        	// a valid square. Lets see if its good
        	queue<pair<int, int> > q;
        	q.emplace(i, j);
        	upto++;
        	seen[i][j] = upto;
        	dis[i][j] = 0;
        	bool bad = false;
        	while (!q.empty())
        	{
        		int x = q.front().first;
        		int y = q.front().second;
        	//	printf("%d %d - %d %d\n", x, y, i, j);
        		q.pop();
        		if (enemy(x,y)) bad = true;
        		if (dis[x][y] == 2) continue;
        		for (int k = 0; k < 8; k++)
        		{
        			int i = x + bc_Direction_dx((bc_Direction)k);
    	    		int j = y + bc_Direction_dy((bc_Direction)k);
    	    		if (!existsOnMap(i, j)) continue;
    	    		if (seen[i][j] != upto)
    	    		{
    	    			seen[i][j] = upto;
    	    			dis[i][j] = dis[x][y]+1;
    	    			q.emplace(i, j);
    	    		}
        		}
        	}
        	if (!bad)
        	{
        		good.push_back(l);
        	}
        }
        if (good.empty()) 
        {
        	// TODO: Do a secondary check. For now we just say they're all good.
        	printf("no good\n");
        	for (int l = 0; l <= 8; l++)
        	{
        		int i = x + bc_Direction_dx((bc_Direction)l);
    	  	  	int j = y + bc_Direction_dy((bc_Direction)l);
        		if (!existsOnMap(i, j)) continue;
        		good.push_back(l);
        	}
        }
        if (!bc_GameController_is_move_ready(gc, id))
        {
        	good.clear();
        	good.push_back(8);
        }
        random_shuffle(good.begin(), good.end());
        int mxdis = 0;
        atDis[0].emplace_back(x, y);
        upto++;
        seen[x][y] = upto;
        firstdir[x][y] = 8;
        // this is a weird bfs/dijkstra
        for (int k = 0; k <= mxdis; k++)
        {
        	for (auto f : atDis[k])
        	{
        		int i = f.first;
        		int j = f.second;
        		int d = (i-x)*(i-x) + (j-y)*(j-y); // distanced squared to (i, j)
        		if (enemy(i, j))
        		{
        			//printf("ENEMY %d\n", d);
        			// check all good squares. Can they reach here
        			for (int l : good)
        			{
        				int a = x + bc_Direction_dx((bc_Direction)l);
        				int b = y + bc_Direction_dy((bc_Direction)l);
        				int dis = (a-i)*(a-i) + (b-j)*(b-j);
        				if (dis <= bc_Unit_attack_range(unit) && dis >= 10)
        				{
        					// Can attack. Move to (a, b), attack (i, j)
        					if (l != 8)
        					{
        						if (bc_GameController_can_move(gc, id, (bc_Direction)l))
        						{
        						//	printf("moving\n");
        							bc_GameController_move_robot(gc, id, (bc_Direction)l);
        						}
        						else printf("ERROR: Can't move\n");
        					}
        					if (bc_GameController_is_attack_ready(gc, id))
        					{
        						bc_MapLocation* loc = new_bc_MapLocation(myPlanet, i, j);
        						assert(bc_GameController_has_unit_at_location(gc, loc));
        						bc_Unit* enemy = bc_GameController_sense_unit_at_location(gc, loc);
        						uint16_t enemyid = bc_Unit_id(enemy);
        						if (bc_GameController_can_attack(gc, id, enemyid))
        						{
        							bc_GameController_attack(gc, id, enemyid);
        						}
        						else printf("ERROR: Can't attack\n");
        						delete_bc_MapLocation(loc);
        						delete_bc_Unit(enemy);
        					}
        					return;
        				}
        			}
        			if (d > bc_Unit_attack_range(unit))
        			{
        				// we'll move closer
        				int k = firstdir[i][j];
        				for (auto l : good)
        				{
        					if (k == l)
        					{
        						//printf("Moving closer to far away enemy\n");
        						if (l != 8)
        						{
        							if (bc_GameController_can_move(gc, id, (bc_Direction)l))
        							{
        							//	printf("moving\n");
        								bc_GameController_move_robot(gc, id, (bc_Direction)l);
        							}
        							else printf("ERROR: Can't move\n");
        						}
        						return;
        					}
        				}
        				
        			} 
        		}
        		for (int l = 0; l < 8; l++)
        		{
        			int a = i + bc_Direction_dx((bc_Direction)l);
    	  	  		int b = j + bc_Direction_dy((bc_Direction)l);
    	  	  		if (!existsOnMap(a, b)) continue;
    	  	  		if (seen[a][b] != upto)
    	  	  		{
    	  	  			seen[a][b] = upto;
    	  	  			int dis = (a-x)*(a-x) + (b-y)*(b-y); // distanced squared to (a, b)
    	  	  			mxdis = max(mxdis, dis);
    	  	  			atDis[dis].emplace_back(a, b);
    	  	  			if (firstdir[i][j] == 8) firstdir[a][b] = l;
    	  	  			else firstdir[a][b] = firstdir[i][j];
    	  	  		}
        		}
        	}
        	atDis[k].clear();
        }
        // There is nowhere that is both good and safe for me to go
        // So i'll just randomly go
        // TODO: Still try to attack or something
   //     printf("Randomly moving ranger\n");
        assert(good.size());
        int l = good[rand()%good.size()];
        if (l != 8)
        {
        	if (bc_GameController_can_move(gc, id, (bc_Direction)l))
        	{
        		bc_GameController_move_robot(gc, id, (bc_Direction)l);
        	}
        	else printf("ERROR: Can't move\n");
        }
	}
};
RangerStrat dealWithRangers;

int main() 
{
    printf("Player C++ bot starting\n");

    srand(420);

    bc_Direction dir = North;
    bc_Direction opposite = bc_Direction_opposite(dir);
    check_errors();

    printf("Opposite direction of %d: %d\n", dir, opposite);

    assert(opposite == South);

    printf("Connecting to manager...\n");

    bc_GameController *gc = new_bc_GameController();

    if (check_errors()) 
    {
        printf("Failed, dying.\n");
        exit(1);
    }
    printf("Connected!\n");
    bc_Planet myPlanet = bc_GameController_planet(gc);
    mars.init(gc);
    earth.init(gc);
    if (myPlanet == Mars)
    {
    	mars.optimalsquare(false);
    }
    bc_PlanetMap* map = bc_GameController_starting_map(gc, myPlanet);
    int myPlanetR = bc_PlanetMap_height_get(map);
    int myPlanetC = bc_PlanetMap_width_get(map);

    int nWorkers;

    bc_Team currTeam = bc_GameController_team(gc);
    bc_Team enemyTeam = (currTeam == Red ? Blue : Red);

    while (true) 
    {
        uint32_t round = bc_GameController_round(gc);
        printf("Round %d\n", round);
        if (round == 1) //start researching rockets
        {
            printf("Trying to queue research... status: ");
            printf("%d\n", bc_GameController_queue_research(gc, Rocket));
            bc_GameController_queue_research(gc, Ranger);
        }
        if (myPlanet == Mars)
        {
            mineKarboniteOnMars(gc);
            fflush(stdout);
            bc_GameController_next_turn(gc);
            continue;
        }
        bc_MapLocation* loc = new_bc_MapLocation(myPlanet, 0, 0);
        for (int i = 0; i < myPlanetC; ++i)
        {
            for (int j = 0; j < myPlanetR; ++j)
            {
                // detect an enemy factory here
                bc_MapLocation_x_set(loc, i);
                bc_MapLocation_y_set(loc, j);

                // NOTE: Temporary workaround of
                // has_unit_at_location
                bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, loc);
                if (unit)
                {
                    bc_UnitType unitType = bc_Unit_unit_type(unit);
                    
                    // if it's an enemy and a factory or rocket:
                    // take note
                    if (bc_Unit_team(unit) != currTeam)
                    {
                        if (unitType == Factory) enemyFactory[i][j] = 1;
                        if (unitType == Rocket) enemyRocket[i][j] = 1;
                    }

                    delete_bc_Unit(unit);
                }
            }
        }
        delete_bc_MapLocation(loc);
        dealWithRangers.init(gc, myPlanet, currTeam);
        // clear the set of occupied directions
        // for factories
        dirAssigned.clear();

        bc_VecUnit *units = bc_GameController_my_units(gc);
        int len = bc_VecUnit_len(units);
        for (int i = 0; i < len; i++) 
        {
            bc_Unit *unit = bc_VecUnit_index(units, i);
            bc_UnitType unitType = bc_Unit_unit_type(unit);
            uint16_t id = bc_Unit_id(unit);
            bc_Location* loc = bc_Unit_location(unit);

            // if this unit has died during this turn
            if (!unit) goto loopCleanup;

            if (unitType == Worker)
            {
                if (assignedStructure.find(id) != assignedStructure.end() &&
                    bc_Location_is_on_map(loc))
                {
                    bc_MapLocation* mapLoc = bc_Location_map_location(loc);

                    uint16_t structureid = assignedStructure[id];
                    bc_Unit *structure = bc_GameController_unit(gc, structureid);

                    if (structure && !bc_Unit_structure_is_built(structure))
                    {
                        if (dirAssigned.find(structureid) == dirAssigned.end()) dirAssigned[structureid] = 0;

                        // build the structure if we can
                        if (bc_GameController_can_build(gc, id, structureid))
                        {
                            bc_GameController_build(gc, id, structureid);
                        }

                        dirAssigned[structureid]++;

                        delete_bc_Unit(structure);
                        delete_bc_MapLocation(mapLoc);
                        goto loopCleanup; // i'm sorry
                    }

                    // if structure is dead or finished:
                    // structure no longer needs units assigned
                    reqAssignees[assignedStructure[id]] = 0;

                    // this unit no longer has an assigned structure
                    assignedStructure.erase(id);

                    delete_bc_Unit(structure);
                    delete_bc_MapLocation(mapLoc);
                }

                // Build a structure next to us
                // (test code)
                if (round == 1) createBlueprint(gc, unit, id, 3, East, Factory);

                // Don't spawn rockets yet
                // if (round == 150 || round == 250) createBlueprint(gc, unit, id, 1, North, Rocket);
                
                // Let's try and get into an adjacent rocket.
                // (Again, test code.)
                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                bc_VecUnit* adjRockets = bc_GameController_sense_nearby_units_by_type(gc, mapLoc, 2, Rocket);
                int len = bc_VecUnit_len(adjRockets);
                for (int j = 0; j < len; ++j)
                {
                    bc_Unit* rocket = bc_VecUnit_index(adjRockets, j);
                    uint16_t rocketid = bc_Unit_id(rocket);
                    if (bc_Unit_team(rocket) == currTeam &&
                        bc_GameController_can_load(gc, rocketid, id))
                    {
                        printf("Loading into a random rocket...\n");
                        bc_GameController_load(gc, rocketid, id);

                        delete_bc_Unit(rocket);
                        break;
                    }

                    delete_bc_Unit(rocket);
                }

                delete_bc_MapLocation(mapLoc);
                delete_bc_VecUnit(adjRockets);
            }
            // Rocket launch code (for testing, doesn't actually take anything into account)
            // Notice that, at present, because the VecUnits isn't actually sorted,
            // this might launch before every adjacent worker is in.
            else if (unitType == Rocket)
            {
                if (!bc_Unit_structure_is_built(unit)) goto loopCleanup;

                mars.updateKarboniteAmount(gc);
                pair<int, int> landingLocPair = mars.optimalsquare();
                printf("%d %d\n", landingLocPair.first, landingLocPair.second);
                bc_MapLocation* landingLoc = new_bc_MapLocation(Mars, landingLocPair.first, landingLocPair.second);
                if (bc_GameController_can_launch_rocket(gc, id, landingLoc)) //and now lets take off
                {
                    printf("Launching...\n");
                    bc_GameController_launch_rocket(gc, id, landingLoc);
                }
                else printf("Launch FAILED\n");
                delete_bc_MapLocation(landingLoc);
            }
            // For now we just spawn units in equal proportions.
            else if (unitType == Factory)
            {
                // Let's build some stuff.
                // Why not.
                if (!bc_Unit_structure_is_built(unit)) goto loopCleanup;

                bc_UnitType type = (rand() % 3 ? (rand() % 2 ? Knight : Ranger) : Mage);

                if (bc_GameController_can_produce_robot(gc, id, type))
                {
                    bc_GameController_produce_robot(gc, id, type);
                }

                for (int j = 0; j < 8; ++j)
                {
                    if (bc_GameController_can_unload(gc, id, (bc_Direction)j))
                    {
                        bc_GameController_unload(gc, id, (bc_Direction)j);
                    }
                }
            }
            else if (unitType == Knight)
            {
                // if we are in a garrison or space:
                // wow, not much you can do now
                if (bc_Location_is_in_garrison(loc) ||
                    bc_Location_is_in_space(loc)) goto loopCleanup;

                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                bc_MapLocation* nearestEnemy;
                bc_Direction dir;
                tie(nearestEnemy, dir) = findNearestEnemy(gc, currTeam, map, mapLoc, 50, false);

                if (!nearestEnemy)
                {
                    // Let's move in a random direction
                    // (that we can move in) for now.
                    vector<int> availableDir;
                    for (int i = 0; i < 8; ++i)
                    {
                        if (bc_GameController_can_move(gc, id, (bc_Direction)i))
                        {
                            availableDir.push_back(i);
                        }
                    }
                    if (availableDir.size())
                    {
                        dir = (bc_Direction)availableDir[rand() % availableDir.size()];
                    }
                    else
                    {
                        dir = North; // we can't do anything
                    }
                }

                if (nearestEnemy && bc_MapLocation_is_adjacent_to(mapLoc, nearestEnemy))
                {
                    // The knight is adjacent to the nearest enemy.
                    // If it's diagonally adjacent,
                    // we'll move in such a way as to make it
                    // vertically or horizontally adjacent.
                    if (dir == Northeast || dir == Southeast ||
                        dir == Southwest || dir == Northwest)
                    {
                        if (bc_GameController_is_move_ready(gc, id))
                        {
                            // subtract 1 from dir:
                            // this'll get us to a position
                            // where we're adjacent to the enemy
                            dir = (bc_Direction)((int)dir - 1);
                            if (bc_GameController_can_move(gc, id, dir))
                            {
                                bc_GameController_move_robot(gc, id, dir);
                            }
                            else
                            {
                                // try the other direction corresponding
                                // to the same diagonal
                                dir = (bc_Direction)(((int)dir + 2) % 8);
                                if (bc_GameController_can_move(gc, id, dir))
                                {
                                    bc_GameController_move_robot(gc, id, dir);
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (bc_GameController_can_move(gc, id, dir) &&
                        bc_GameController_is_move_ready(gc, id))
                    {
                        bc_GameController_move_robot(gc, id, dir);
                    }
                }

                // if we can attack the nearest enemy:
                // do it
                // (note: for other units than Knight,
                //  the nearest enemy might not actually be
                //  the nearest enemy, so you'll have to handle,
                //  for instance, attacking other enemies
                //  that get in the way)
                if (nearestEnemy)
                {
                    bc_Unit* enemy = bc_GameController_sense_unit_at_location(gc, nearestEnemy);
                    uint16_t enemyid = bc_Unit_id(enemy);
                    if (bc_GameController_can_attack(gc, id, enemyid) &&
                        bc_GameController_is_attack_ready(gc, id))
                    {
                        bc_GameController_attack(gc, id, enemyid);
                    }

                    delete_bc_Unit(enemy);
                }

                delete_bc_MapLocation(mapLoc);
                if (nearestEnemy) delete_bc_MapLocation(nearestEnemy);
            }
            else if (unitType == Ranger)
            {
            	dealWithRangers.findNearestEnemy(gc, unit);
            }
            // TODO:
            // We need some way of making sure that our
            // units don't prevent factories from being able
            // to unload.
            // Also mages don't care about friendly fire.
            else if (unitType == Mage)
            {
                // if we are in a garrison or space:
                // wow, not much you can do now
                if (bc_Location_is_in_garrison(loc) ||
                    bc_Location_is_in_space(loc)) goto loopCleanup;

                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                bc_MapLocation *nearestRanger, *nearestOverall;
                bc_Direction dir;

                // check if there's a ranger nearby (rip)
                // note that we check slightly outside our vision range
                // because someone else might see a ranger
                tie(nearestRanger, dir) = findNearestEnemy(gc, currTeam, map, mapLoc, 50, true, Ranger);

                tie(nearestOverall, dir) = findNearestEnemy(gc, currTeam, map, mapLoc, 50, false);

                // Check if the nearby ranger's squared distance
                // is at most 3 times the distance of the nearest unit overall
                // otherwise we might have higher priorities

                if (nearestRanger &&
                    bc_MapLocation_distance_squared_to(mapLoc, nearestRanger) <=
                    3 * bc_MapLocation_distance_squared_to(mapLoc, nearestOverall))
                {
                    // RUN AWAY
                    // Choose the direction that maximises our distance
                    // from the ranger
                    if (bc_GameController_is_move_ready(gc, id))
                    {
                        bc_Direction bestDir = North; int maxDist = -1;
                        for (int d = 0; d < 8; ++d)
                        {
                            bc_MapLocation* newLoc = bc_MapLocation_add(mapLoc, (bc_Direction)d);

                            int newDist = bc_MapLocation_distance_squared_to(newLoc, nearestRanger);
                            if (newDist > maxDist &&
                                bc_GameController_can_move(gc, id, (bc_Direction)d))
                            {
                                bestDir = (bc_Direction)d;
                                maxDist = newDist;
                            }

                            delete_bc_MapLocation(newLoc);
                        }
                        if (bc_GameController_can_move(gc, id, bestDir))
                        {
                            bc_GameController_move_robot(gc, id, bestDir);
                        }
                    }

                    // attack!!
                    bc_Unit* enemy = bc_GameController_sense_unit_at_location(gc, nearestRanger);
                    uint16_t enemyid = bc_Unit_id(enemy);
                    if (bc_GameController_can_attack(gc, id, enemyid) &&
                        bc_GameController_is_attack_ready(gc, id))
                    {
                        bc_GameController_attack(gc, id, enemyid);
                    }

                    delete_bc_Unit(enemy);
                }

                // Try to shoot at the nearest overall enemy now.
                // This also occurs if we were unable to attack
                // a ranger that we're running away from.
                if (nearestOverall)
                {
                    bc_Unit* enemy = bc_GameController_sense_unit_at_location(gc, nearestOverall);
                    uint16_t enemyid = bc_Unit_id(enemy);
                    if (bc_GameController_can_attack(gc, id, enemyid) &&
                        bc_GameController_is_attack_ready(gc, id))
                    {
                        bc_GameController_attack(gc, id, enemyid);
                    }

                    delete_bc_Unit(enemy);
                }
                else
                {
                    // nobody nearby.
                    // let's move randomly
                    vector<int> availableDir;
                    for (int i = 0; i < 8; ++i)
                    {
                        if (bc_GameController_can_move(gc, id, (bc_Direction)i))
                        {
                            availableDir.push_back(i);
                        }
                    }
                    if (availableDir.size())
                    {
                        dir = (bc_Direction)availableDir[rand() % availableDir.size()];
                    }
                    else
                    {
                        dir = North; // we can't do anything
                    }
                    if (bc_GameController_can_move(gc, id, dir) &&
                        bc_GameController_is_move_ready(gc, id))
                    {
                        bc_GameController_move_robot(gc, id, dir);
                    }
                }

                delete_bc_MapLocation(mapLoc);
                if (nearestRanger) delete_bc_MapLocation(nearestRanger);
                if (nearestOverall) delete_bc_MapLocation(nearestOverall);
            }

            loopCleanup:
            if (unit)
            {
                delete_bc_Unit(unit);
                delete_bc_Location(loc);
            }
        }

        // For each structure which doesn't have enough workers:
        // duplicate workers around it to fulfil the quota
        // (we don't 'steal' workers that are passing by since
        //  that might stuff us up)
        // also make sure no units are at the locations
        for (auto P : dirAssigned)
        {
            uint16_t structureid; int numAssigned;
            tie(structureid, numAssigned) = P;

            bc_Unit *structure = bc_GameController_unit(gc, structureid);
            bc_Location *loc = bc_Unit_location(structure);
            bc_MapLocation *mapLoc = bc_Location_map_location(loc);

            if (numAssigned < reqAssignees[structureid])
            {
                bc_MapLocation *structureAdj[8];
                uint16_t workerid[8];
                bool hasworker[8];
                for (int d = 0; d < 8; ++d) {
                    structureAdj[d] = bc_MapLocation_add(mapLoc, (bc_Direction)d);
                    bc_Unit *unit = bc_GameController_sense_unit_at_location(gc, structureAdj[d]);
                    hasworker[d] = !!unit;
                    if (unit) workerid[d] = bc_Unit_id(unit);
                    delete_bc_Unit(unit);
                }

                // we don't actually really need to be smart about this.
                // let's do a greedy which is suboptimal but whatever.
                for (int d = 0; d < 8; ++d) if (bc_GameController_is_occupiable(gc, structureAdj[d]))
                {
                    if (numAssigned >= reqAssignees[structureid]) break;
                    for (int j = 0; j < 8; ++j) if (hasworker[j])
                    {
                        // we might be able to clone from robot j
                        // to robot d.
                        if (bc_MapLocation_is_adjacent_to(structureAdj[d], structureAdj[j]))
                        {
                            bc_Direction dir = bc_MapLocation_direction_to(structureAdj[j], structureAdj[d]);
                            if (bc_GameController_can_replicate(gc, workerid[j], dir)) {
                                bc_GameController_replicate(gc, workerid[j], dir);

                                // assign the new one to the structure
                                bc_Unit *newUnit = bc_GameController_sense_unit_at_location(gc, structureAdj[d]);
                                uint16_t newid = bc_Unit_id(newUnit);
                                assignedStructure[newid] = structureid;

                                delete_bc_Unit(newUnit);

                                numAssigned++;
                                break;
                            }
                        }
                    }
                }

                for (int d = 0; d < 8; ++d) delete_bc_MapLocation(structureAdj[d]);
            }

            delete_bc_Unit(structure);
            delete_bc_Location(loc);
            delete_bc_MapLocation(mapLoc);
        }

        delete_bc_VecUnit(units);

        fflush(stdout);

        bc_GameController_next_turn(gc);
    }

    delete_bc_PlanetMap(map);
    delete_bc_GameController(gc);
}
