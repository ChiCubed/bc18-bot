#include <cstdio>
#include <vector>
#include <queue>
#include <algorithm>
#include <utility>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
using namespace std;
#define mp make_pair
#define pb push_back
#define emb emplace_back
// oh god
#define this it
#include <bc.h>
#undef this

// good practices
#include "voronoi.cpp"



#define USE_PERMANENTLY_ASSIGNED_WORKERS 0

#define USE_SNIPE 1



#define RIP_IN_PIECES_MSG ('R' * 256 * 256 + 'I' * 256 + 'P')
#define HealerRange 100

bool weExisted, shouldGoInGroup;
bool earthIsDead = false;
bool enemyIsDead = false;
bool needMoreWorkers;
bool opponentExists;
int lastRocket, savingForRocket, savingForFactory;
set<pair<int, int> > fullRockets, toDelete; // stores rockets that are full.
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


// gets the 'distance' between two ratios
// i.e. an approximate measure of similarity
// (note: only consistent if one of the input variables is;
//  it's implemented this way so we can use integers)
int getRatioDistance(vector<int> A, vector<int> B)
{
    assert (A.size() == B.size()); // Ratios are same length

    int res = 0;

    int sumA = 0, sumB = 0;
    for (int &e : A) sumA += e;
    for (int &e : B) sumB += e;

    for (int i = 0; i < A.size(); ++i)
    {
        res += abs(A[i] * sumB - B[i] * sumA);
    }

    return res;
}


struct MarsSTRUCT //contains a more readable map of mars, as well as the code to find where to send a rocket
{
    int r, c, numberOfWorkers;
    int mars[60][60]; // 0 = passable, 1 = unpassable
    int karbonite[60][60]; // number of karbonite at each square at some time
    int seen[60][60], upto, dis[60][60], comp[60][60], compsize[3000], hasrocket[60][60], robotthatlead[60][60], firstdir[60][60], hasUnit[60][60];
    int rocketsInComp[3000], workersInComp[3000];
    queue<pair<int, pair<int, int> > > takenSquaresQueue;
    set<pair<int, int> > taken;
    unordered_set<int> landedRockets;
    bc_AsteroidPattern* asteroidPattern;
    void initKarboniteAm(bc_GameController* gc, bc_PlanetMap* planetMap)
    {
    	for (int i = 0; i < c; i++)
    	{
    		for (int j = 0; j < r; j++)
    		{
    			bc_MapLocation* mapLoc = new_bc_MapLocation(Mars, i, j);
				karbonite[i][j] = bc_PlanetMap_initial_karbonite_at(planetMap, mapLoc);
    		}
    	}
    }
    void init(bc_GameController* gc)
    {
        bc_Planet bc_Planet_mars = Mars;
        bc_PlanetMap* map = bc_GameController_starting_map(gc, bc_Planet_mars);
        r = bc_PlanetMap_height_get(map);
        c = bc_PlanetMap_width_get(map);
        initKarboniteAm(gc, map);
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
        q[e++] = mp(s, e);
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
                        q[e++] = mp(s, e);
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

    pair<int, int> optimalsquare(int round, bool count = true)
    {   
        // if we flew to a square a while ago, we can assume the rocket unloaded
        while (!takenSquaresQueue.empty() && takenSquaresQueue.front().first + 170 >= round)
        {   
            hasrocket[takenSquaresQueue.front().second.first][takenSquaresQueue.front().second.second] = false;
            takenSquaresQueue.pop();
        }
        int A = 1; //constant, representing how much weighting the amount of karbonite should have
        int B = 1; //weighting of size of component
        int C = 1; //weighting of amount of neighbours
        // A (and possibly also B/C) could be dependent on total available karbonite/average comp size/whatever
        int K = 5; //dist to consider for karbonite
        upto++;
        int mxCompSize = 0;
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++) 
            { 
                if (!mars[i][j]) 
                {
                    findcomps(i, j, i*r+j);
                    if (comp[i][j] != i*r+j) continue;
                    mxCompSize = max(mxCompSize, compsize[i*r+j]);
                }
            }
        }
        pair<pair<int, int>, pair<int, int> > best = mp(mp(-999999999, 0), mp(0, 0));
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {   
                if (mars[i][j] || hasrocket[i][j] || compsize[comp[i][j]]*3 < mxCompSize) continue;
                int nearbykar = bfs(i, j, K);
                int compsz = rocketsInComp[comp[i][j]]/compsize[comp[i][j]];
                compsz = -compsz;
                int numnei = numneighbours(i, j);
                int rating = A*nearbykar + B*compsz + C*numnei;
                best = max(best, mp(mp(rating, rand()), mp(i, j))); //rand is so that it picks randomly among ties, rather than picking highest x and y
            }
        }
        if (count) hasrocket[best.second.first][best.second.second] = true;
        if (count) rocketsInComp[comp[best.second.first][best.second.second]]++;
        if (count) takenSquaresQueue.emplace(round, best.second);
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
                if (bc_GameController_can_sense_location(gc, loc))
                {
                	karbonite[i][j] = bc_GameController_karbonite_at(gc, loc);
                }
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
    int seen[60][60], upto, dis[60][60], robotthatlead[60][60], firstdir[60][60], hasUnit[60][60];
    set<pair<int, int> > taken;
    int amKarbonite;
    void initKarboniteAm(bc_GameController* gc, bc_PlanetMap* planetMap)
    {
    	for (int i = 0; i < c; i++)
    	{
    		for (int j = 0; j < r; j++)
    		{
    			bc_MapLocation* mapLoc = new_bc_MapLocation(Earth, i, j);
				karbonite[i][j] = bc_PlanetMap_initial_karbonite_at(planetMap, mapLoc);
    		}
    	}
    }
    void init(bc_GameController* gc)
    {
        bc_Planet bc_Planet_earth = Earth;
        bc_PlanetMap* map = bc_GameController_starting_map(gc, bc_Planet_earth);
        r = bc_PlanetMap_height_get(map);
        c = bc_PlanetMap_width_get(map);
        initKarboniteAm(gc, map);
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
    void updateKarboniteAmount(bc_GameController* gc) // this function will be used until the asteroid thing is fixed
    { // I think this only has karbonite within the viewable range ......... rip.....
        bc_MapLocation* loc;
        amKarbonite = 0;
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {
                loc = new_bc_MapLocation(Earth, i, j);
                if (bc_GameController_can_sense_location(gc, loc))
                {
                	karbonite[i][j] = bc_GameController_karbonite_at(gc, loc);
                }
                delete_bc_MapLocation(loc);
                if (karbonite[i][j]) amKarbonite++;
            }
        }
    }
};

MarsSTRUCT mars;
EarthSTRUCT earth;

bool enemyFactory[60][60], enemyRocket[60][60];
// which rocket / factory a unit is assigned to
map<uint16_t, uint16_t> assignedStructure;

#if USE_PERMANENTLY_ASSIGNED_WORKERS
// a permanent assignment to a structure
// for a worker. if the structure gets damaged,
// the worker will rebuild.
map<uint16_t, uint16_t> permanentAssignedStructure;
// what direction the assigned structure is in, for a
// permanently assigned unit
map<uint16_t, bc_Direction> permanentAssignedDirection;
// the type of the assigned structure
map<uint16_t, bc_UnitType> permanentAssignedType;
#endif

// which directions are assigned for a structure
map<uint16_t, int> dirAssigned;
// how many workers a rocket / factory should have
map<uint16_t, int> reqAssignees;

// initial distance to an enemy unit
// int distToInitialEnemy[60][60];

// initial enemy locations
vector<pair<int, int>> initialEnemies;

// pairs of {distance to enemy, unit}
// stores which units are too close to the enemy
vector<pair<int, bc_Unit*>> tooClose;

vector<pair<int, int>> targetEnemies;

// returns true if successful
// the main worker will become
// permanently assigned to the structure
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
        printf("Deleting unit\n");
        bc_UnitType unitType = bc_Unit_unit_type(tmp);
        if (unitType == Rocket || unitType == Factory)
        {
        	printf("ERROR: We are destroying a rocket/factory in order to blueprint a rocket/factory!\n");
        }
        uint16_t id = bc_Unit_id(tmp);
        bc_GameController_disintegrate_unit(gc, id);
        delete_bc_Unit(tmp);
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

        #if USE_PERMANENTLY_ASSIGNED_WORKERS
        permanentAssignedStructure[id] = structureid;
        permanentAssignedDirection[id] = dir;
        permanentAssignedType[id] = type;
        #endif
        
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
int mxWorkersOnEarth = 12;
int extraEarlyGameWorkers = 0;
void mineKarboniteOnEarth(bc_GameController* gc, int totalUnits, int round)
{
    earth.taken.clear();
    earth.updateKarboniteAmount(gc);
    bc_VecUnit *units = bc_GameController_my_units(gc); 
    int len = bc_VecUnit_len(units);
    vector<bc_Unit*> canMove;
    vector<bc_Unit*> workers; // all workers
    for (int l = 0; l < len; l++) 
    {
        bc_Unit *unit = bc_VecUnit_index(units, l);
        bc_UnitType unitType = bc_Unit_unit_type(unit);
        uint16_t id = bc_Unit_id(unit);
        bc_Location* loc = bc_Unit_location(unit);
        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
        int x = bc_MapLocation_x_get(mapLoc);
        int y = bc_MapLocation_y_get(mapLoc);
        earth.hasUnit[x][y] = 1;
        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
        if (unitType == Worker)
        {
            // deep copy the unit to help with stuff
            workers.push_back(bc_Unit_clone(unit));
            if (assignedStructure.find(id) == assignedStructure.end())
            {
                for (int i = 8; i >= 0; i--)
                {
                    if (bc_GameController_can_harvest(gc, id, (bc_Direction)i))
                    {
                     //   printf("harvesting\n");
                        bc_GameController_harvest(gc, id, (bc_Direction)i); // harvest the karbonite
                        break; // we can only harvest 1 per turn :(
                    }
                }
                if (true || bc_GameController_is_move_ready(gc, id)) canMove.push_back(unit); // consider all workers, rather than the ones that can move
                else delete_bc_Unit(unit);
            }
            else delete_bc_Unit(unit);
        }
        else delete_bc_Unit(unit);
    }
    int amWorkers = totalUnits/20;
    bool shouldReplicate = true;
    if (round > 200)
    {
        // if we are dying;
       // if (totalUnits < (canMove.size()*2)-2 || savingForRocket) shouldReplicate = false; shouldReplicate = false;
        mxWorkersOnEarth = min(12, earth.amKarbonite);
        mxWorkersOnEarth = min(mxWorkersOnEarth, totalUnits/2);
    }
    amWorkers = min(amWorkers + 6, mxWorkersOnEarth);
    amWorkers += extraEarlyGameWorkers;
    amWorkers = min(amWorkers, mxWorkersOnEarth + 6);
    random_shuffle(workers.begin(), workers.end());
    if (canMove.size() < amWorkers && shouldReplicate) // not enough workers...
    {
        vector<bc_Unit*> newCanMove;
        // we can duplicate even those workers
        // which are assigned to a structure
        for (auto unit : workers)
        {
            uint16_t id = bc_Unit_id(unit);
            for (int i = 0; i < 8; i++)
            {
                if (bc_GameController_can_replicate(gc, id, (bc_Direction)i))
                {
                    printf("Duplicated worker\n");
                    bc_GameController_replicate(gc, id, (bc_Direction)i);

                    // get the new worker
                    bc_Location* loc = bc_Unit_location(unit);
                    bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                    bc_MapLocation* newLoc = bc_MapLocation_add(mapLoc, (bc_Direction)i);
                    bc_Unit* newUnit = bc_GameController_sense_unit_at_location(gc, newLoc);
                    assert(newUnit);
                    uint16_t newid = bc_Unit_id(newUnit);

                    if (true || bc_GameController_is_move_ready(gc, id)) newCanMove.push_back(newUnit); // consider all workers, rather than the ones that can move

                    delete_bc_Location(loc);
                    delete_bc_MapLocation(mapLoc);
                    delete_bc_MapLocation(newLoc);

                    break;
                }
            }
            if (newCanMove.size() + canMove.size() >= amWorkers) break;
        }

        for (auto unit : workers) delete_bc_Unit(unit);
        workers.clear();

        canMove.insert(canMove.end(), newCanMove.begin(), newCanMove.end());
    }
    while (!canMove.empty()) // where should we move to
    {
        bool worked = false;
        earth.upto++;
        queue<pair<int, int> > q;
        for (int i = 0; i < canMove.size(); i++)
        {
            auto unit = canMove[i];
            bc_Location* loc = bc_Unit_location(unit);
            bc_MapLocation* mapLoc = bc_Location_map_location(loc);
            int x = bc_MapLocation_x_get(mapLoc);
            int y = bc_MapLocation_y_get(mapLoc);
            earth.seen[x][y] = earth.upto;
            earth.robotthatlead[x][y] = i;
            earth.firstdir[x][y] = 8;
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
            if (earth.karbonite[x][y] && earth.taken.find(mp(x, y)) == earth.taken.end())
            {
                // We've reached some karbonite. Send the robot in question here;
                earth.taken.insert(mp(x, y));

                bc_Unit* unit = canMove[earth.robotthatlead[x][y]];
                canMove.erase(canMove.begin()+earth.robotthatlead[x][y]);
                int dir = earth.firstdir[x][y];
                uint16_t id = bc_Unit_id(unit);
                // Update the location of robots;
                bc_Location* loc = bc_Unit_location(unit);
                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                int i = bc_MapLocation_x_get(mapLoc);
                int j = bc_MapLocation_y_get(mapLoc);
                delete_bc_MapLocation(mapLoc);
                delete_bc_Location(loc);
                earth.hasUnit[i][j] = 0;
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
                earth.hasUnit[i][j] = 1;
                delete_bc_Unit(unit);
                worked = true;
                break;
            }   
            if (earth.earth[x][y] || (earth.hasUnit[x][y] && earth.firstdir[x][y] != 8)) continue;
            for (int l = 0; l < 8; l++) // consider going this direction
            {
                int i = x + bc_Direction_dx((bc_Direction)l);
                int j = y + bc_Direction_dy((bc_Direction)l);
                if (i >= 0 && i < earth.c && j >= 0 && j < earth.r && earth.seen[i][j] != earth.upto)
                {
                    earth.seen[i][j] = earth.upto;
                    earth.robotthatlead[i][j] = earth.robotthatlead[x][y];
                    if (earth.firstdir[x][y] == 8) earth.firstdir[i][j] = l;
                    else earth.firstdir[i][j] = earth.firstdir[x][y];
                    q.emplace(i, j);
                }
            }
        }
        if (!worked)
        {
            for (auto unit : canMove)
            {
                // randomly walk
                uint16_t id = bc_Unit_id(unit);
                for (int i = 0; i < 8; i++)
                {
                    int l = rand()%8;
                    if (bc_GameController_is_move_ready(gc, id) && bc_GameController_can_move(gc, id, (bc_Direction)l))
                    {
                        bc_GameController_move_robot(gc, id, (bc_Direction)l);
                    }   
                }
                delete_bc_Unit(unit);
            }
            canMove.clear();
        }
    }
}
void mineKarboniteOnMars(bc_GameController* gc, int round) // Controls the mining of Karbonite on mars
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
                 //   printf("harvesting\n");
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
            bc_VecUnitID *garrisonUnits = bc_Unit_structure_garrison(unit);
            int len = bc_VecUnitID_len(garrisonUnits);
            delete_bc_VecUnitID(garrisonUnits);
            if (!len)
            {
                printf("Trying to disintegrate\n");
                bc_GameController_disintegrate_unit(gc, id);
            }
            else printf("%d\n", len);
            delete_bc_Unit(unit);
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
        if ((mars.workersInComp[mars.comp[x][y]] < min(5*mars.rocketsInComp[mars.comp[x][y]], 2+mars.compsize[mars.comp[x][y]]/5) && (round <= 450 || round >= 750)) ||
            round >= 750 ||
            earthIsDead) // we want to have 2 workers per rocket that landed
        {
            vector<int> dir;
            for (int i = 0; i < 8; i++) dir.pb(i);
            random_shuffle(dir.begin(), dir.end());
            for (int i : dir)
            {
                if (bc_GameController_can_replicate(gc, id, (bc_Direction)i))
                {
                    printf("Duplicated worker\n");
                    bc_GameController_replicate(gc, id, (bc_Direction)i);
                    mars.workersInComp[mars.comp[x][y]]++;
                    if (mars.workersInComp[mars.comp[x][y]] >= min(5*mars.rocketsInComp[mars.comp[x][y]], 2+mars.compsize[mars.comp[x][y]]/5) && round < 750) break;
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
            if (mars.karbonite[x][y] && mars.taken.find(mp(x, y)) == mars.taken.end())
            {
                // We've reached some karbonite. Send the robot in question here;
                mars.taken.insert(mp(x, y));

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
            for (auto unit : canMove)
            {
                // randomly walk
                uint16_t id = bc_Unit_id(unit);
                for (int i = 0; i < 8; i++)
                {
                    int l = rand()%8;
                    if (bc_GameController_is_move_ready(gc, id) && bc_GameController_can_move(gc, id, (bc_Direction)l))
                    {
                        bc_GameController_move_robot(gc, id, (bc_Direction)l);
                    }   
                }
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
    int hasEnemy[60][70];
    int r, c, seen[60][60], upto, dis[60][60];
    bc_Planet myPlanet;
    bc_Team myTeam;
    bc_GameController* gc;
    int numOpWorkers, dontAttackWorkers;
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
    int enemy(int x, int y)
    {
        bc_MapLocation* loc = new_bc_MapLocation(myPlanet, x, y);
        if (bc_GameController_has_unit_at_location(gc, loc))
        {
            bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, loc);
            if (bc_Unit_team(unit) != myTeam)
            {
                int id = bc_Unit_id(unit);
                delete_bc_MapLocation(loc);
                delete_bc_Unit(unit);
                return id+1;
            }
            delete_bc_Unit(unit);
        }
        delete_bc_MapLocation(loc);
        return false;
    }
    bool enemytype(int x, int y, bc_UnitType type)
    {
        bc_MapLocation* loc = new_bc_MapLocation(myPlanet, x, y);
        if (bc_GameController_has_unit_at_location(gc, loc))
        {
            bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, loc);
            if (bc_Unit_team(unit) != myTeam && bc_Unit_unit_type(unit) == type)
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
    bool existsOnMapNotFriendly(int x, int y)
    {
        if (x < 0 || y < 0 || x >= c || y >= r) return false;
        if (myPlanet == Earth) return !earth.earth[x][y];
        else return !mars.mars[x][y];
    }
    bool oppositionIsDead()
    {
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {
                if (!existsOnMapNotFriendly(i, j)) continue;
                bc_MapLocation* loc = new_bc_MapLocation(myPlanet, i, j);
                if (!bc_GameController_can_sense_location(gc, loc)) { delete_bc_MapLocation(loc); return false; }
                delete_bc_MapLocation(loc);
                if (enemy(i, j)) return false;
            }
        }
        return true;
    }
    vector<int> findGood(bc_GameController* gc, int id, int x, int y)
    {
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
            //  printf("%d %d - %d %d\n", x, y, i, j);
                q.pop();
                if (enemytype(x,y,Knight)) bad = true;
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
          //  printf("no good\n");
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
        if (good.empty())
        {
            good.push_back(8);
        }
        random_shuffle(good.begin(), good.end());
        return good;
    }
    pair<int, int> storeLoc[60][60][12];
    vector<pair<pair<int, int>, int> > distances;
    vector<pair<pair<int, int>, int> > mageDistances, knightDistances;
    void pushDistances()
    {
        // These are vertical ranges which make up a rangers attack range
        // TODO: Check this 1e9 times to make sure it isn't wrong
        // The first number is x displacement, 2nd is y displacement, 3rd is length-1
        distances.emb(mp(7, -1), 2);
        distances.emb(mp(6, -3), 6);
        distances.emb(mp(5, -5), 10);
        distances.emb(mp(4, -5), 10);
        distances.emb(mp(3, -6), 4);
        distances.emb(mp(3, 2), 4);
        distances.emb(mp(2, -6), 3);
        distances.emb(mp(2, 3), 3);
        distances.emb(mp(1, -7), 3);
        distances.emb(mp(1, 4), 3);
        distances.emb(mp(0, -7), 3);
        distances.emb(mp(0, 4), 3);
        distances.emb(mp(-7, -1), 2);
        distances.emb(mp(-6, -3), 6);
        distances.emb(mp(-5, -5), 10);
        distances.emb(mp(-4, -5), 10);
        distances.emb(mp(-3, -6), 4);
        distances.emb(mp(-3, 2), 4);
        distances.emb(mp(-2, -6), 3);
        distances.emb(mp(-2, 3), 3);
        distances.emb(mp(-1, -7), 3);
        distances.emb(mp(-1, 4), 3);

        // These are vertical ranges which make up a mage attack range
        // TODO: Check this 1e9 times to make sure it isn't wrong
        mageDistances.emb(mp(5, -2), 4);
        mageDistances.emb(mp(4, -3), 6);
        mageDistances.emb(mp(3, -4), 8);
        mageDistances.emb(mp(2, -5), 10);
        mageDistances.emb(mp(1, -5), 10);
        mageDistances.emb(mp(0, -5), 10);
        mageDistances.emb(mp(-5, -2), 4);
        mageDistances.emb(mp(-4, -3), 6);
        mageDistances.emb(mp(-3, -4), 8);
        mageDistances.emb(mp(-2, -5), 10);
        mageDistances.emb(mp(-1, -5), 10);

        // Knight distances for javelin
        knightDistances.emb(mp(3, -1), 2);
        knightDistances.emb(mp(2, -2), 4);
        knightDistances.emb(mp(1, -3), 6);
        knightDistances.emb(mp(0, -3), 6);
        knightDistances.emb(mp(-3, -1), 2);
        knightDistances.emb(mp(-2, -2), 4);
        knightDistances.emb(mp(-1, -3), 6);
    }
    void preCompLoc()
    {
        // precomp in an array for faster querytime
        numOpWorkers = 0;
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++) 
            { 
                hasEnemy[i][j] = enemy(i, j); 
                numOpWorkers += enemytype(i, j, Worker); 
            }
        }
        if (myPlanet == Mars && numOpWorkers >= 8)
        {
            printf("Too many opposition workers\n");
            // no point prioritising them, they can dupe
            dontAttackWorkers = true;
        }
        else dontAttackWorkers = false;
        // precomps for each segment of sz up to 11, is there an enemy and where that enemy is
        for (int i = 0; i < c; i++)
        {
            for (int j = 0; j < r; j++)
            {
                pair<int, int> curr = mp(-1, -1);
                for (int k = j; k < 11+j; k++)
                {
                    if (hasEnemy[i][k])
                    {
                        curr = mp(i, k);
                    }
                    storeLoc[i][j][k-j] = curr;
                }
            }
        }
    }
    
    bool mageAttack(bc_GameController* gc, bc_Unit* unit, int x, int y, int id)
    {
        bool attacked = false;
        bc_MapLocation* mapLoc;
        vector<pair<int, int> > bestv;
        int bestWeighting = 0;
        bc_Unit* newUnit;
        if (bc_GameController_is_attack_ready(gc, id))
        {
            for (int i = max(0, x-6); i < min(c, x+7); i++)
            {
                for (int j = max(0, y-6); j < min(r, y+7); j++)
                {
                    if ((i-x)*(i-x) + (j-y)*(j-y) > bc_Unit_attack_range(unit)) continue;
                    mapLoc = new_bc_MapLocation(myPlanet, i, j);
                    if (!bc_GameController_has_unit_at_location(gc, mapLoc)) continue;
                    delete_bc_MapLocation(mapLoc);
                    int weighting = 0;
                    for (int l = 0; l <= 8; l++)
                    {
                        int x = i + bc_Direction_dx((bc_Direction)l);
                        int y = j + bc_Direction_dy((bc_Direction)l);
                        mapLoc = new_bc_MapLocation(myPlanet, x, y);
                        if (!bc_GameController_has_unit_at_location(gc, mapLoc)) continue;
                        newUnit = bc_GameController_sense_unit_at_location(gc, mapLoc);
                        delete_bc_MapLocation(mapLoc);
                        bc_UnitType unitType = bc_Unit_unit_type(newUnit);
                        bc_Team unitTeam = bc_Unit_team(newUnit);
                        if (dontAttackWorkers)
                        {
                            int am = 1;
                            if (unitType == Rocket || unitType == Factory) am = 8;
                            else if (unitType != Worker) am = 6;
                            if (unitTeam == myTeam) { am = -am; am*=2; }
                            weighting += am;
                        }
                        else
                        {
                            int am = 1;
                            if (unitType == Rocket || unitType == Factory) am = 2;
                            if (unitTeam == myTeam) { am = -am; am*=2; }
                            weighting += am;
                        }
                        delete_bc_Unit(newUnit);
                    }
                    if (weighting > bestWeighting)
                    {
                        bestv.clear();
                        bestWeighting = weighting;
                    }
                    if (weighting == bestWeighting)
                    {
                        bestv.emb(i, j);
                    }
                }
            }
            if (bestWeighting)
            {
                pair<int, int> best = bestv[rand()%bestv.size()];
                mapLoc = new_bc_MapLocation(myPlanet, best.first, best.second);
                newUnit = bc_GameController_sense_unit_at_location(gc, mapLoc);
                int enemyid = bc_Unit_id(newUnit);
                delete_bc_MapLocation(mapLoc);
                delete_bc_Unit(newUnit);
                if (bc_GameController_can_attack(gc, id, enemyid))
                {   
                //    printf("Mage attacked! %d\n", bestWeighting);
                    attacked = true;
                    bc_GameController_attack(gc, id, enemyid);
                }     
                else printf("Error: Can't attack MAGE\n");
            }
        }
        return attacked;

    }
    // for rangers: does all their work
    bool rangerAttack(bc_GameController* gc, bc_Unit* unit, int x, int y, int id)
    {
        bc_UnitType type = bc_Unit_unit_type(unit);
        bool attacked = false;
        if (bc_GameController_is_attack_ready(gc, id))
        {
            // Note:
            // First we check if we can just attack
            // the regular nearest enemy.
            // If not, let's do this search
            // and thus attack a semi-arbitrary
            // nearest enemy.
            pair<int, int> nearestEnemyLoc = Voronoi::closestEnemy[x][y];
            int enemyid = enemy(nearestEnemyLoc.first, nearestEnemyLoc.second);
            if (enemyid)
            {
                enemyid--;
                if (bc_GameController_can_attack(gc, id, enemyid))
                {
                    attacked = true;
                    bc_GameController_attack(gc, id, enemyid);
                    return attacked;
                }
            }


            pair<int, int> target = mp(-1, -1);
            pair<int, int> workerTarget = mp(-1, -1);
            if (type == Ranger)
            {
                for (auto a : distances)
                {
                    int i = x+a.first.first;
                    int j = y+a.first.second;
                    int sz = a.second;
                    if (i < 0 || i >= c) continue;
                    if (j >= r) continue;
                    if (j < 0)
                    {
                        sz += j;
                        j = 0;
                    }
                    if (sz < 0) continue;
                    pair<int, int> canAttack = storeLoc[i][j][sz];
                    if (dontAttackWorkers)
                    {
                        if (enemytype(i, j, Worker))
                        {
                            if (workerTarget.first == -1) workerTarget = canAttack;
                            else if (rand()%3 == 0) workerTarget = canAttack;
                        }
                        else
                        {
                            if (target.first == -1) target = canAttack;
                            else if (rand()%3 == 0) target = canAttack;
                        }
                    }
                    else
                    {
                        if (canAttack.first != -1)
                        {
                            if (target.first == -1) target = canAttack;
                            else if (rand()%3 == 0) target = canAttack;
                        }
                    }
                }
            }
            else printf("WRONG TYPE\n");
            if (target.first != -1)
            {
                int enemyid = enemy(target.first, target.second);
                if (enemyid)
                {
                    enemyid--;
                  //  printf("%d\n", (target.first-x)*(target.first-x) + (target.second-y)*(target.second-y));
                    if (bc_GameController_can_attack(gc, id, enemyid))
                    {   
                        attacked = true;
                        bc_GameController_attack(gc, id, enemyid);
                    }     
                    else printf("Error: Can't attack\n");
                }
                else printf("Enemy has already been killed\n");
            }
            if (workerTarget.first != -1)
            {
                int enemyid = enemy(workerTarget.first, workerTarget.second);
                if (enemyid)
                {
                    enemyid--;
                    if (bc_GameController_can_attack(gc, id, enemyid))
                    {   
                        attacked = true;
                        bc_GameController_attack(gc, id, enemyid);
                    }     
                    else printf("Error: Can't attack\n");
                }
                else printf("Enemy has already been killed\n");
            }
        }
        return attacked;
    }
    void findNearestEnemy(bc_GameController* gc, bc_Unit* unit, bool allowMove = true)
    {   
        uint16_t id = bc_Unit_id(unit);
        bc_Location* loc = bc_Unit_location(unit);
        if (!bc_Location_is_on_planet(loc, Mars) && !bc_Location_is_on_planet(loc, Earth)) { delete_bc_Location(loc); return; }
        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
        int x = bc_MapLocation_x_get(mapLoc), y = bc_MapLocation_y_get(mapLoc);
        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
        bc_UnitType unitType = bc_Unit_unit_type(unit);
        bool attacked = false;
        if (unitType == Ranger) attacked = rangerAttack(gc, unit, x, y, id);
        else if (unitType == Mage) attacked = mageAttack(gc, unit, x, y, id);
        if (!bc_GameController_is_move_ready(gc, id) || !allowMove) return;

        // NOTE:
        // Swarm dynamic takes over automatically and sets allowMove to false
      //  vector<int> good = findGood(gc, id, x, y);
        //bool isGood[9];
      //  fill_n(isGood, 9, false);
       // for (int a : good) isGood[a] = true;
        // otherwise, lets randomly move;
     //   int l = good[rand()%good.size()];
        // don't need to consider good squares - we can't see any enemies if we're walking
        vector<int> a;
        for (int i = 0; i < 8; i++) a.push_back(i);
        random_shuffle(a.begin(), a.end());
        for (auto l : a)
        {
            if (bc_GameController_can_move(gc, id, (bc_Direction)l))
            {
                bc_GameController_move_robot(gc, id, (bc_Direction)l);
                return;
            }
        }
    }  
    void doJavelinAttack(bc_GameController* gc, bc_Unit* unit)
    {
        uint16_t id = bc_Unit_id(unit);
        bc_Location* loc = bc_Unit_location(unit);
        if (!bc_Location_is_on_planet(loc, Mars) && !bc_Location_is_on_planet(loc, Earth)) { delete_bc_Location(loc); return; }
        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
        int x = bc_MapLocation_x_get(mapLoc), y = bc_MapLocation_y_get(mapLoc);
        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
        vector<pair<int, int> > targets;
        for (auto a : knightDistances)
        {
            int i = x+a.first.first;
            int j = y+a.first.second;
            int sz = a.second;
            if (i < 0 || i >= c) continue;
            if (j >= r) continue;
            if (j < 0)
            {
                sz += j;
                j = 0;
            }
            if (sz < 0) continue;
            pair<int, int> canAttack = storeLoc[i][j][sz];
            if (canAttack.first != -1)
            {
                targets.pb(canAttack);
            }
        }
        random_shuffle(targets.begin(), targets.end());
        for (int i = 0; i < targets.size(); i++)
        {
            pair<int, int> target = targets[i];
            int enemyid = enemy(target.first, target.second);
            if (enemyid)
            {
                enemyid--;
                //  printf("%d\n", (target.first-x)*(target.first-x) + (target.second-y)*(target.second-y));
                if (bc_GameController_can_javelin(gc, id, enemyid))
                {   
                    printf("Javelin attack!\n");
                    bc_GameController_javelin(gc, id, enemyid);
                    break;
                }     
                else printf("Error: Can't javelin\n");
            }
        }
    }    
};
RangerStrat dealWithRangers;
int healthAtSquare[60][60];
void compHealth(bc_GameController* gc)
{
	bc_MapLocation* mapLoc;
	bc_Unit* unit;
	for (int i = 0; i < dealWithRangers.c; i++)
	{
		for (int j = 0; j < dealWithRangers.r; j++)
		{
			healthAtSquare[i][j] = 0;
			mapLoc = new_bc_MapLocation(dealWithRangers.myPlanet, i, j);
            if (!bc_GameController_has_unit_at_location(gc, mapLoc)) { delete_bc_MapLocation(mapLoc); continue; }
            bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, mapLoc);
            bc_Team team = bc_Unit_team(unit);
            delete_bc_MapLocation(mapLoc);
            bc_UnitType type = bc_Unit_unit_type(unit);
            if (team != dealWithRangers.myTeam || type == Rocket || type == Factory || type == Healer) { delete_bc_Unit(unit); continue; }
            healthAtSquare[i][j] = bc_Unit_max_health(unit) - bc_Unit_health(unit);
            delete_bc_Unit(unit);
		}
	}
}
void healerHeal(bc_GameController* gc, bc_Unit* unit, bool allowMove = true) // healerHeal sounds bad ... but its like rangerAttack and mageAttack
{
	uint16_t id = bc_Unit_id(unit);
    bc_Location* loc = bc_Unit_location(unit);
    if (!bc_Location_is_on_planet(loc, Mars) && !bc_Location_is_on_planet(loc, Earth)) { delete_bc_Location(loc); return; }
    bc_MapLocation* mapLoc = bc_Location_map_location(loc);
    int x = bc_MapLocation_x_get(mapLoc), y = bc_MapLocation_y_get(mapLoc);
    delete_bc_Location(loc);
    delete_bc_MapLocation(mapLoc);
    vector<pair<int, int> > bestv;
    int bestWeighting = 0;
    // for now, check just check all squares
    if (bc_GameController_is_heal_ready(gc, id))
    {
        for (int i = max(0, x-6); i < min(dealWithRangers.c, x+7); i++)
        {
            for (int j = max(0, y-6); j < min(dealWithRangers.r, y+7); j++)
            {
                if ((i-x)*(i-x) + (j-y)*(j-y) > 30) continue;
                int weighting = healthAtSquare[i][j];
                if (weighting > bestWeighting)
                {
                    bestv.clear();
                    bestWeighting = weighting;
                }
                if (weighting == bestWeighting)
                {
                    bestv.emb(i, j);
                }
            }
        }
        if (bestWeighting)
        {
            pair<int, int> best = bestv[rand()%bestv.size()];
            mapLoc = new_bc_MapLocation(dealWithRangers.myPlanet, best.first, best.second);
            bc_Unit* newUnit = bc_GameController_sense_unit_at_location(gc, mapLoc);
            int friendid = bc_Unit_id(newUnit);
            delete_bc_MapLocation(mapLoc);
            delete_bc_Unit(newUnit);
            if (bc_GameController_can_heal(gc, id, friendid))
            {   
            	healthAtSquare[best.first][best.second] += bc_Unit_damage(unit);
                bc_GameController_heal(gc, id, friendid);
            }     
            else printf("Error: Can't heal\n");
        }
    }

    if (!bc_GameController_is_move_ready(gc, id) || !allowMove) return;
    vector<int> a;
    for (int i = 0; i < 8; i++) a.push_back(i);
    random_shuffle(a.begin(), a.end());
    for (auto l : a)
    {
        if (bc_GameController_can_move(gc, id, (bc_Direction)l))
        {
            bc_GameController_move_robot(gc, id, (bc_Direction)l);
            return;
        }
    }
}
pair<bc_Unit*, bc_Direction> factoryLocation(bc_GameController* gc, bc_VecUnit* units, int len, bc_UnitType structure, int round)
{
    // placed it in a function so that it can be used for rockets as well.
            bc_Unit* bestUnit;
            bc_Direction bestDir = Center;
            int maxDist = 99999;
            int maxAdjFree = -1;
            for (int i = 0; i < len; ++i)
            {
                bc_Unit* unit = bc_VecUnit_index(units, i);
                uint16_t id = bc_Unit_id(unit);
                bc_UnitType unitType = bc_Unit_unit_type(unit);

                if (unitType != Worker
                #if USE_PERMANENTLY_ASSIGNED_WORKERS
                    || permanentAssignedStructure.find(id) != permanentAssignedStructure.end()
                #endif
                   )
                {
                    continue;
                }

                // make sure we don't assign one that's the only one
                // assigned to its structure
                if (assignedStructure.find(id) != assignedStructure.end())
                {
                    continue;
                }

                bc_Location* loc = bc_Unit_location(unit);
                if (bc_Location_is_in_garrison(loc) ||
                    bc_Location_is_in_space(loc))
                {
                    delete_bc_Location(loc);

                    continue;
                }
                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                int x = bc_MapLocation_x_get(mapLoc);
                int y = bc_MapLocation_y_get(mapLoc);

                bc_Team enemyTeam = (bc_GameController_team(gc) == Red ? Blue : Red);

                for (int d = 0; d < 8; ++d)
                {
                	// we consider killing our own units
                	int dx = bc_Direction_dx((bc_Direction)d);
                    int dy = bc_Direction_dy((bc_Direction)d);
                	bool shouldConsider = false;
                	if (bc_GameController_can_blueprint(gc, id, structure, (bc_Direction)d)) shouldConsider = true;
                	else if (round > 50)
                	{
                		bc_MapLocation* loc = new_bc_MapLocation(dealWithRangers.myPlanet, x+dx, y+dy);
                		if (bc_GameController_has_unit_at_location(gc, loc))
                		{
                			bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, loc);
                			if (bc_Unit_team(unit) == dealWithRangers.myTeam)
                			{
                				if (bc_Unit_unit_type(unit) != Factory && bc_Unit_unit_type(unit) != Rocket)
                				{
                					// lets consider killing this unit
                					shouldConsider = true;
                				}
                			}
                			delete_bc_Unit(unit);
                		}
                		delete_bc_MapLocation(loc);
                	}
                    if (shouldConsider &&
                        bc_UnitType_blueprint_cost(structure) <= bc_GameController_karbonite(gc))
                    {
                        // int dist = distToInitialEnemy[x+dx][y+dy];

                        // dist is the number of enemy units
                        // within 50 steps of the factory.
                        int dist = 0;
                        if (structure == Factory)
                        {
	                        bc_VecUnit* enemyUnits = bc_GameController_sense_nearby_units_by_team(gc, mapLoc, 50, enemyTeam);

    	                    // NOTE:
        	                // don't count enemy workers
            	            int eLen = bc_VecUnit_len(enemyUnits);
                	        for (int j = 0; j < eLen; ++j)
                    	    {
                        	    bc_Unit* eunit = bc_VecUnit_index(enemyUnits, j);
                            	bc_UnitType etype = bc_Unit_unit_type(eunit);
	                            if (etype != Worker &&
    	                            etype != Rocket &&
        	                        etype != Factory)
            	                {
                	                dist++;
                    	        }

	                            delete_bc_Unit(eunit);
    	                    }
        	                delete_bc_VecUnit(enemyUnits);
            			}
            			else
            			{
            				dist = -Voronoi::disToClosestEnemy[x+dx][y+dy];
            			}            
                        if (dist < maxDist)
                        {
                            maxDist = dist;
                        }
                        if (dist == maxDist)
                        {
                            // determine if it has more free adj squares
                            int freeAdj = 0;
                            bc_MapLocation* newLoc = bc_MapLocation_add(mapLoc, (bc_Direction)d);
                            for (int d2 = 0; d2 < 8; ++d2)
                            {
                                bc_MapLocation* newNewLoc = bc_MapLocation_add(newLoc, (bc_Direction)d2);
                                if (bc_GameController_is_occupiable(gc, newNewLoc))
                                {
                                    freeAdj++;
                                }
                                delete_bc_MapLocation(newNewLoc);
                            }
                            delete_bc_MapLocation(newLoc);

                            if (freeAdj > maxAdjFree)
                            {
                                maxAdjFree = freeAdj;
                                bestUnit = unit;
                                bestDir = (bc_Direction)d;
                            }
                        }
                    }
                }

                delete_bc_Location(loc);
                delete_bc_MapLocation(mapLoc);

                // make sure not to delete the unit here
            }
    return mp(bestUnit, bestDir);
}
map<int, int> numWorkersInRocket, numKnightsInRocket, numRangersInRocket, numMagesInRocket, numHealersInRocket;
void tryToLoadIntoRocket(bc_GameController* gc, bc_Unit* unit, bc_Location* loc, bool gettingCloseToFlood)
{
    // tries to load this unit into any adjacent rocket

    // if we're close to the flood, we just admit any unit type

    bc_UnitType unitType = bc_Unit_unit_type(unit);
    uint16_t id = bc_Unit_id(unit);
    if (bc_Location_is_on_planet(loc, Earth) && !bc_Location_is_in_garrison(loc))
    {
        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
        bc_VecUnit* vecUnits = bc_GameController_sense_nearby_units_by_type(gc, mapLoc, 4, Rocket);
        int len = bc_VecUnit_len(vecUnits);
        for (int i = 0; i < len; i++)
        {
            bc_Unit* rocketUnit = bc_VecUnit_index(vecUnits, i);
            int rocketId = bc_Unit_id(rocketUnit);
            if (assignedStructure.find(id) != assignedStructure.end() && assignedStructure[id] != rocketId) continue;
            if (numWorkersInRocket.find(rocketId) == numWorkersInRocket.end())
            {
                numWorkersInRocket[rocketId] = numKnightsInRocket[rocketId] = numRangersInRocket[rocketId] = numMagesInRocket[rocketId] = 0;
            }
     	    if (unitType == Worker && numWorkersInRocket[rocketId] >= 2) continue;
            if (bc_GameController_can_load(gc, rocketId, id))
            {
                if (unitType == Worker) 
                {
                	// if we can replicate this round, or if we have already this round
                	if (bc_Unit_ability_heat(unit) < 10)
                	{
                		numWorkersInRocket[rocketId]++;
                		for (int l = 0; l < 8; l++)
                		{
                			if (bc_GameController_can_replicate(gc, id, (bc_Direction)l))
                			{
                				bc_GameController_replicate(gc, id, (bc_Direction)l);
                				printf("Loading working into rocket (after duping)\n");
                				bc_GameController_load(gc, rocketId, id);
                				goto rocketCleanup; // RIP, not more goto's
                			}
                		}
                		// ok, lets try disintegrating a unit
                		bc_Location* loc = bc_Unit_location(unit);
                		if (!bc_Location_is_on_planet(loc, Earth))
                		{
                			delete_bc_Location(loc);
                			printf("Error: there is a big issue with our code.\n");
                			goto rocketCleanup;
                		}
                		bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                		int x = bc_MapLocation_x_get(mapLoc);
                		int y = bc_MapLocation_x_get(mapLoc);
                		delete_bc_Location(loc);
                		delete_bc_MapLocation(mapLoc);
                		for (int l = 0; l < 8; l++)
                		{
                			int i = x + bc_Direction_dx((bc_Direction)l);
                			int j = y + bc_Direction_dy((bc_Direction)l);
                			if (!dealWithRangers.existsOnMapNotFriendly(i, j)) continue;
                			mapLoc = new_bc_MapLocation(dealWithRangers.myPlanet, i, j);
                			if (bc_GameController_has_unit_at_location(gc, mapLoc))
                			{
                				bc_Unit* dedUnit = bc_GameController_sense_unit_at_location(gc, mapLoc);
                				bc_UnitType dedType = bc_Unit_unit_type(dedUnit);
                				if (dedType != Rocket && dedType != Factory && dedType != Worker && bc_Unit_team(dedUnit) == dealWithRangers.myTeam)
                				{
                					uint16_t dedid = bc_Unit_id(dedUnit);
                					bc_GameController_disintegrate_unit(gc, dedid);
                					printf("Disintegrated unit %d %d : me %d %d\n", i, j, x, y);
                					if (bc_GameController_can_replicate(gc, id, (bc_Direction)l))
       		         				{
            	    					bc_GameController_replicate(gc, id, (bc_Direction)l);
                						printf("Loading working into rocket (after duping and killing)\n");
                						delete_bc_Unit(dedUnit);
                						delete_bc_MapLocation(mapLoc);
                						bc_GameController_load(gc, rocketId, id);
                						goto rocketCleanup; // RIP, not more goto's
                					}
                					else printf("ERROR: Couldn't replicate :(\n");
                					delete_bc_MapLocation(mapLoc);
                					delete_bc_Unit(dedUnit);
                					break;
                				}
                				delete_bc_Unit(dedUnit);
                			}
                			delete_bc_MapLocation(mapLoc);
                		}
                		printf("Couldn't replicate ... meh, might as well load\n");
                		bc_GameController_load(gc, rocketId, id);
                	}
                	else if (bc_Unit_ability_heat(unit) >= bc_Unit_ability_cooldown(unit))
                	{
                		numWorkersInRocket[rocketId]++;
                		printf("Loading working into rocket (already duped)\n");
                		bc_GameController_load(gc, rocketId, id);
                		goto rocketCleanup;
                	}
                	else goto rocketCleanup;
                }
                else if (unitType == Knight) numKnightsInRocket[rocketId]++;
                else if (unitType == Ranger) numRangersInRocket[rocketId]++;
                else if (unitType == Mage) numMagesInRocket[rocketId]++;
                else if (unitType == Healer) numHealersInRocket[rocketId]++;
                printf("Loading into rocket\n");
                bc_GameController_load(gc, rocketId, id);
            }
            rocketCleanup: 
            delete_bc_Unit(rocketUnit);

        }
        delete_bc_MapLocation(mapLoc);
        delete_bc_VecUnit(vecUnits);
    }
}


// for loading into rockets post round 700 ish
int distToRocket[60][60];
bc_Direction directionFromRocket[60][60];
map<int, int> numWorkersGoingToRocket, numKnightsGoingToRocket, numHealersGoingToRocket, numMagesGoingToRocket;
set<int> rocketThatLed[52][52];
void bfsRocketDists(bc_GameController* gc, int am = 8)
{
    // Determine the distance of every square
    // from a rocket
    map<int, int> rocketThatLedMap;
    for (int i = 0; i < 52; ++i) for (int j = 0; j < 52; ++j)
    {
        distToRocket[i][j] = 2e9;
        rocketThatLed[i][j].clear();
    }

    queue<pair<int, pair<int, int>>> Q;
    numWorkersGoingToRocket.clear();
    numKnightsGoingToRocket.clear();
    numHealersGoingToRocket.clear();
    bc_VecUnit* units = bc_GameController_my_units(gc);
    int len = bc_VecUnit_len(units);
    for (int i = 0; i < len; ++i)
    {
        bc_Unit* unit = bc_VecUnit_index(units, i);

        if (bc_Unit_unit_type(unit) == Rocket && bc_Unit_structure_is_built(unit))
        {
            // don't count rockets which are full
            bc_VecUnitID* unitsInside = bc_Unit_structure_garrison(unit);
            int inLen = bc_VecUnitID_len(unitsInside);
            delete_bc_VecUnitID(unitsInside);

            if (inLen == bc_Unit_structure_max_capacity(unit))
            {
                continue;
            }

            bc_Location* loc = bc_Unit_location(unit);
            if (!bc_Location_is_on_planet(loc, Earth))
            {
                delete_bc_Location(loc);
                continue;
            }

            bc_MapLocation* mapLoc = bc_Location_map_location(loc);
            int x = bc_MapLocation_x_get(mapLoc);
            int y = bc_MapLocation_y_get(mapLoc);
            int id = bc_Unit_id(unit);
            Q.push({id, make_pair(x, y)});
            distToRocket[x][y] = 0;
            rocketThatLed[x][y].insert(id);
            rocketThatLedMap[id] = numWorkersGoingToRocket[id] = numKnightsGoingToRocket[id] = numHealersGoingToRocket[id] = numMagesGoingToRocket[id] = 0;
            if (numWorkersInRocket.find(id) == numWorkersInRocket.end()) numWorkersInRocket[id] = 0;
            if (numHealersInRocket.find(id) == numHealersInRocket.end()) numHealersInRocket[id] = 0;
            if (numKnightsInRocket.find(id) == numKnightsInRocket.end()) numKnightsInRocket[id] = 0;
            delete_bc_Location(loc);
            delete_bc_MapLocation(mapLoc);
        }

        delete_bc_Unit(unit);
    }
    delete_bc_VecUnit(units);
    bc_MapLocation* loc;
    while (Q.size())
    {
        int x, y, rocket;
        rocket = Q.front().first; x = Q.front().second.first; y = Q.front().second.second;
       	Q.pop();

        if (earth.earth[x][y]) continue; // unpassable
        if (rocketThatLedMap[rocket] >= am) continue;
        for (int d = 0; d < 8; ++d)
        {
            bc_Direction dir = (bc_Direction)d;
            int dx = bc_Direction_dx(dir);
            int dy = bc_Direction_dy(dir);
            if (x + dx < 0 || x + dx >= earth.c ||
                y + dy < 0 || y + dy >= earth.r) continue;
            if (dealWithRangers.enemy(x+dx, y+dy) || !dealWithRangers.existsOnMapNotFriendly(x+dx, y+dy)) continue;
            if (rocketThatLed[x+dx][y+dy].find(rocket) != rocketThatLed[x+dx][y+dy].end()) continue;
            if (rocketThatLed[x+dx][y+dy].empty())
            {
            	distToRocket[x+dx][y+dy] = distToRocket[x][y] + 1;
            	directionFromRocket[x+dx][y+dy] = dir;
            }   
            if (dealWithRangers.friendly(x+dx, y+dy) && rocketThatLed[x+dx][y+dy].empty())
            {
            	loc = new_bc_MapLocation(dealWithRangers.myPlanet, x+dx, y+dy);
            	bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, loc);                   
                bc_UnitType unitType = bc_Unit_unit_type(unit);                       
                delete_bc_Unit(unit);
                delete_bc_MapLocation(loc);
                int id = rocket;
                if (unitType == Worker && numWorkersInRocket[id] + numWorkersGoingToRocket[id] >= 2)  { directionFromRocket[x+dx][y+dy] = (bc_Direction)8; }
                else if (unitType == Healer && numHealersInRocket[id] + numHealersGoingToRocket[id] >= 3) { directionFromRocket[x+dx][y+dy] = (bc_Direction)8;}
               	else if (unitType == Knight && numKnightsInRocket[id] + numKnightsGoingToRocket[id] >= 2) { directionFromRocket[x+dx][y+dy] = (bc_Direction)8;}
               	else if (unitType == Mage && numMagesInRocket[id] + numMagesGoingToRocket[id] >= 3) { directionFromRocket[x+dx][y+dy] = (bc_Direction)8;}
               	else if (unitType != Rocket && unitType != Factory)
               	{
               		// all good 
               		rocketThatLedMap[id]++;
               		if (unitType == Worker) numWorkersGoingToRocket[id]++;
               		if (unitType == Healer) numHealersGoingToRocket[id]++;
               		if (unitType == Knight) numKnightsGoingToRocket[id]++;
               		if (unitType == Mage) numMagesGoingToRocket[id]++;
               	}
            }
            rocketThatLed[x+dx][y+dy].insert(rocket);
            Q.push({rocket, make_pair(x+dx, y+dy)});
        }
    }
}


void launchAllFullRockets(bc_GameController* gc, int round)
{
	mars.updateKarboniteAmount(gc);
	printf("Launching all %lu rockets\n", fullRockets.size());
	for (auto it = fullRockets.begin(); it != fullRockets.end(); ++it)
	{	
		int x = it->first;
		int y = it->second;
		if (toDelete.find(make_pair(x, y)) != toDelete.end()) continue;
		bc_MapLocation* mapLoc = new_bc_MapLocation(Earth, x, y);
		if (bc_GameController_has_unit_at_location(gc, mapLoc))
		{
			bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, mapLoc);
			if (bc_Unit_unit_type(unit) == Rocket)
			{
				uint16_t id = bc_Unit_id(unit);
				pair<int, int> landingLocPair = mars.optimalsquare(round);
		      	bc_MapLocation* landingLoc = new_bc_MapLocation(Mars, landingLocPair.first, landingLocPair.second);
    		    printf("%d %d\n", landingLocPair.first, landingLocPair.second);
        		if (bc_GameController_can_launch_rocket(gc, id, landingLoc)) //and now lets take off
	    	    {
    	    	    printf("Launching. . .\n");
        	    	bc_GameController_launch_rocket(gc, id, landingLoc);
		        }
    		    else printf("Launch FAILED\n");
        		delete_bc_MapLocation(landingLoc);
			}
			else printf("No rocket here!\n");
			delete_bc_Unit(unit);
		}
		else printf("Somehow this rocket was killed (this is an issue)\n");
		delete_bc_MapLocation(mapLoc);
	}
	fullRockets.clear();
}

queue<pair<int, int>> unitMovementBFSQueue;
bool unitMovementSeen[60][60];
int unitMovementDist[60][60];


queue<pair<int, int>> initialKarboniteBFSQueue;
bool initialKarboniteSeen[60][60];
int initialKarboniteDist[60][60];

vector<pair<int, pair<int, int>>> rangersByDistance;
vector<pair<int, pair<int, int>>> structuresToSnipe;
vector<pair<int, pair<int, pair<int, int>>>> unitsToSnipe;


unordered_set<uint16_t> currSnipers;
queue<pair<uint16_t, int>> sniperEndTimes;


// For calculation of sizes etc of blobs
pair<int, int> parentUnitLocation[60][60];
int minEuclideanDist[60][60];
int maxEuclideanDist[60][60];
int sizeOfBlob[60][60];

pair<int, int> findParent(int x, int y)
{
    return (parentUnitLocation[x][y].first != x || parentUnitLocation[x][y].second != y)
         ? parentUnitLocation[x][y] = findParent(parentUnitLocation[x][y].first, parentUnitLocation[x][y].second)
         : mp(x, y);
}

int main() 
{
    printf("Player C++ bot starting\n");
    fflush(stdout);
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
    fflush(stdout);

    bc_Planet myPlanet = bc_GameController_planet(gc);
    mars.init(gc);
    earth.init(gc);
    if (myPlanet == Mars)
    {
        mars.optimalsquare(0, false);
    }
    bc_PlanetMap* map = bc_GameController_starting_map(gc, myPlanet);
    int myPlanetR = bc_PlanetMap_height_get(map);
    int myPlanetC = bc_PlanetMap_width_get(map);

    int nWorkers;

    bc_Team currTeam = bc_GameController_team(gc);
    bc_Team enemyTeam = (currTeam == Red ? Blue : Red);
    srand(420*(int)currTeam);
    dealWithRangers.init(gc, myPlanet, currTeam);
    dealWithRangers.pushDistances();
    // compute initial distance to enemy units
    // for use in naive factory building
    /*
    for (int i = 0; i < 60; ++i)
    {
        for (int j = 0; j < 60; ++j)
        {
            distToInitialEnemy[i][j] = 1e9;
        }
    }
    queue<pair<int,int>> bfsQueue;
    bc_VecUnit* initUnits = bc_PlanetMap_initial_units_get(map);
    int len = bc_VecUnit_len(initUnits);
    for (int i = 0; i < len; ++i)
    {
        bc_Unit* unit = bc_VecUnit_index(initUnits, i);
        bc_Team team = bc_Unit_team(unit);
        if (team == enemyTeam)
        {
            bc_Location* loc = bc_Unit_location(unit);
            bc_MapLocation* mapLoc = bc_Location_map_location(loc);
            int x = bc_MapLocation_x_get(mapLoc);
            int y = bc_MapLocation_y_get(mapLoc);
            bfsQueue.push({x, y});
            distToInitialEnemy[x][y] = 0;
            delete_bc_Location(loc);
            delete_bc_MapLocation(mapLoc);
        }
        delete_bc_Unit(unit);
    }
    delete_bc_VecUnit(initUnits);
    // now for the bfs
    while (bfsQueue.size()) {
        int x, y;
        tie(x, y) = bfsQueue.front(); bfsQueue.pop();
        if (x < 0 || x >= earth.c ||
            y < 0 || y >= earth.r ||
            earth.earth[x][y])
        {
            // unpassable
            continue;
        }
        for (int d = 0; d < 8; ++d)
        {
            int dx = bc_Direction_dx((bc_Direction)d),
                dy = bc_Direction_dy((bc_Direction)d);
            if (distToInitialEnemy[x+dx][y+dy] >
                distToInitialEnemy[x][y] + 1)
            {
                distToInitialEnemy[x+dx][y+dy] = distToInitialEnemy[x][y] + 1;
                bfsQueue.push({x+dx, y+dy});
            }
        }
    }
    */

    bc_VecUnit* initUnits = bc_PlanetMap_initial_units_get(map);
    int len = bc_VecUnit_len(initUnits);
    for (int i = 0; i < len; ++i)
    {
        bc_Unit* unit = bc_VecUnit_index(initUnits, i);

        bc_Location* loc = bc_Unit_location(unit);
        bc_MapLocation* mapLoc = bc_Location_map_location(loc);

        int x = bc_MapLocation_x_get(mapLoc);
        int y = bc_MapLocation_y_get(mapLoc);

        bc_Team team = bc_Unit_team(unit);
        if (team == enemyTeam)
        {
            initialEnemies.push_back({x, y});
        }
        else
        {
            initialKarboniteBFSQueue.push({x, y});
            initialKarboniteSeen[x][y] = 1;
            initialKarboniteDist[x][y] = 0;
        }

        delete_bc_Unit(unit);
        delete_bc_Location(loc);
        delete_bc_MapLocation(mapLoc);
    }
    delete_bc_VecUnit(initUnits);


    // BFS for all the early-game reachable karbonite
    int initialReachableKarbonite = 0;
    if (myPlanet == Earth)
    {
        while (initialKarboniteBFSQueue.size())
        {
            int x, y;
            tie(x, y) = initialKarboniteBFSQueue.front(); initialKarboniteBFSQueue.pop();

            if (earth.earth[x][y]) continue;
            if (initialKarboniteDist[x][y] >= 15) break;

            bc_MapLocation* mapLoc = new_bc_MapLocation(Earth, x, y);
            initialReachableKarbonite += bc_PlanetMap_initial_karbonite_at(map, mapLoc);

            for (int d = 0; d < 8; ++d)
            {
                int dx = bc_Direction_dx((bc_Direction)d);
                int dy = bc_Direction_dy((bc_Direction)d);

                if (x+dx < 0 || x+dx >= myPlanetC || y+dy < 0 || y+dy >= myPlanetR)
                {
                    continue;
                }

                if (initialKarboniteSeen[x+dx][y+dy]) continue;
                initialKarboniteSeen[x+dx][y+dy] = 1;
                initialKarboniteDist[x+dx][y+dy] = initialKarboniteDist[x][y] + 1;
                initialKarboniteBFSQueue.push({x+dx, y+dy});
            }
        }
    }

    printf("Early-game karbonite to harvest: %d\n", initialReachableKarbonite);

    int reqNFactories = min(4 + initialReachableKarbonite / 1200, 6);


    while (true) 
    {
        uint32_t round = bc_GameController_round(gc);
        printf("Round %d\n", round);
        dealWithRangers.preCompLoc();
        if (round == 1) //start researching rockets
        {
            printf("Trying to queue research... status: ");
            bc_GameController_queue_research(gc, Ranger); // 25  25
            bc_GameController_queue_research(gc, Healer); // 25  50
            bc_GameController_queue_research(gc, Ranger); // 100 200
            bc_GameController_queue_research(gc, Rocket); // 200 250
            bc_GameController_queue_research(gc, Ranger); // 250 450
            bc_GameController_queue_research(gc, Healer); // 450 550
            bc_GameController_queue_research(gc, Mage);   // 550 575
            bc_GameController_queue_research(gc, Mage);   // 575 650
            bc_GameController_queue_research(gc, Mage);   // 650 750
            // we don't even use blink...
            // bc_GameController_queue_research(gc, Mage);   // 750 825
            // Why not.
            bc_GameController_queue_research(gc, Healer); // 750 850
        }
        if (!enemyIsDead && myPlanet == Earth && round % 2 == 0)
        {
            if (dealWithRangers.oppositionIsDead()) enemyIsDead = true;
        }
        if (enemyIsDead && round % 20 == 0)
        {
            printf("Enemy is dead!\n");
        }
        if (myPlanet == Mars)
        {
            // Check if Earth is dead (RIP)
            if (round > 50)
            {
                bc_Veci32* earthArr = bc_GameController_get_team_array(gc, Earth);
                // assert (bc_Veci32_len(earthArr) >= 1);
                earthIsDead |= (bc_Veci32_index(earthArr, 0) == RIP_IN_PIECES_MSG);
                delete_bc_Veci32(earthArr);
            }
            mineKarboniteOnMars(gc, round);
        }
        if (myPlanet == Earth)
        {
        	if (round > 50)
        	{
        		bc_Veci32* earthArr = bc_GameController_get_team_array(gc, Earth);
                // assert (bc_Veci32_len(earthArr) >= 1);
                shouldGoInGroup = (bc_Veci32_index(earthArr, 0) == RIP_IN_PIECES_MSG);
                delete_bc_Veci32(earthArr);
        	}
        }
        else
        {
            if (round <= 40)
            {
                extraEarlyGameWorkers = initialReachableKarbonite / 240;
            }
            else
            {
                extraEarlyGameWorkers = 0;
            }

            bc_MapLocation* loc = new_bc_MapLocation(myPlanet, 0, 0);
            opponentExists = false;
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
                            opponentExists = true;
                            if (unitType == Factory) enemyFactory[i][j] = 1;
                            if (unitType == Rocket) enemyRocket[i][j] = 1;
                        }

                        delete_bc_Unit(unit);
                    }
                }
            }
            delete_bc_MapLocation(loc);
        }


        while (sniperEndTimes.size() && sniperEndTimes.front().second < round)
        {
            currSnipers.erase(sniperEndTimes.front().first);
            sniperEndTimes.pop();
        }


        // clear the set of occupied directions
        // for factories
        dirAssigned.clear();

        bc_VecUnit *units = bc_GameController_my_units(gc);

        int len = bc_VecUnit_len(units);

        // Firstly, let's count the number of each unit type
        // note: healers and workers are ignored at the moment
        int nRangers = 0, nKnights = 0, nMages = 0, nFactories = 0, nWorkers = 0, nRockets = 0, nInGarrison = 0, nHealers = 0, nfreeWorkers = 0;
        for (int i = 0; i < len; ++i)
        {
            bc_Unit* unit = bc_VecUnit_index(units, i);
            bc_UnitType unitType = bc_Unit_unit_type(unit);
            bc_Location* loc = bc_Unit_location(unit);
            if (bc_Location_is_in_garrison(loc) && unitType != Worker) nInGarrison++;
            if (unitType == Ranger) nRangers++; 
            if (unitType == Knight) nKnights++;
            if (unitType == Mage) nMages++;
            if (unitType == Factory)
            {
                // only count factories that aren't finished
                // or factories that have a decent amount of health
                // (basic heuristic to rebuild after factories are damaged)
                if (!bc_Unit_structure_is_built(unit) ||
                    bc_Unit_health(unit) >= 150)
                {
                    nFactories++;
                }
            }
            if (unitType == Worker) nWorkers++;
            if (unitType == Rocket) nRockets++;
            if (unitType == Healer) nHealers++;
            if (!bc_Location_is_in_garrison(loc) && unitType == Worker) nfreeWorkers++;
            delete_bc_Location(loc);
            // don't delete unit here: we need units later
        }
        printf("Free %d : %d\n", nfreeWorkers, nWorkers);
        // calculate how many assignees each structure has

        // NOTE:
        // This _MUST_ be done before any calls to
        // factoryLocation are made.
        for (int i = 0; i < len; ++i) 
        {
            bc_Unit* unit = bc_VecUnit_index(units, i);
            bc_UnitType unitType = bc_Unit_unit_type(unit);
            uint16_t id = bc_Unit_id(unit);
            bc_Location* loc = bc_Unit_location(unit);
            if (unitType == Worker && myPlanet == Earth)
            {
                if (assignedStructure.find(id) != assignedStructure.end() &&
                    bc_Location_is_on_map(loc))
                {
                    uint16_t structureid = assignedStructure[id];
                    bc_Unit *structure = bc_GameController_unit(gc, structureid);

                    if (structure &&
                        (!bc_Unit_structure_is_built(structure)
                    #if USE_PERMANENTLY_ASSIGNED_WORKERS
                         || bc_Unit_health(structure) <= 250           
                    #endif
                         ))
                    {
                        if (dirAssigned.find(structureid) == dirAssigned.end()) dirAssigned[structureid] = 0;

                        dirAssigned[structureid]++;
                    }
                    delete_bc_Unit(structure);
                }
            }
            delete_bc_Location(loc);
        }
        if (nFactories < reqNFactories && myPlanet == Earth && round >= 5)
        {
            // Let's find a location for a new factory
            // next to a worker that's as far as possible
            // from any initial enemy unit location.
            pair<bc_Unit*, bc_Direction> bestLoc = factoryLocation(gc, units, len, Factory, round);
            bc_Unit* bestUnit = bestLoc.first;
            bc_Direction bestDir = bestLoc.second;
            if (bestDir != Center)
            {
                // we can build a factory!
                // yaaay
                bc_Location* loc = bc_Unit_location(bestUnit);
                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                bc_VecUnit* nearbyEnemies = bc_GameController_sense_nearby_units_by_team(gc, mapLoc, 40, enemyTeam);
                int len = bc_VecUnit_len(nearbyEnemies);
                if (len < 2) savingForFactory = true;
                else savingForFactory = false;
                uint16_t id = bc_Unit_id(bestUnit);
                delete_bc_MapLocation(mapLoc);
                delete_bc_Location(loc);
                for (int i = 0; i < len; ++i) delete_bc_Unit(bc_VecUnit_index(nearbyEnemies, i));
                delete_bc_VecUnit(nearbyEnemies);
                createBlueprint(gc, bestUnit, id, 3, bestDir, Factory);
            }
            else savingForFactory = false;
        }
        else savingForFactory = false;
        

        // if we have no units left:
        // let's tell Mars to begin worker duplication
        if (myPlanet == Earth && (len == 0)) bc_GameController_write_team_array(gc, 0, RIP_IN_PIECES_MSG);
        if (myPlanet == Mars && len == 0 && weExisted)
        {
        	bc_GameController_write_team_array(gc, 1, RIP_IN_PIECES_MSG);
        }
        else if (myPlanet == Mars) bc_GameController_write_team_array(gc, 1, 0);
        int goToMarsRound = 750 - ((earth.r + earth.c)) - 200;
        if ((round >= goToMarsRound || enemyIsDead) && myPlanet == Earth)
        {
            // compute distances to rockets
            bfsRocketDists(gc);
        }
        else if (myPlanet == Earth) bfsRocketDists(gc, 6);

        // Let's calculate nearest enemies to each point
        // using discount voronoi
        // Let's not include workers because they don't do much

        targetEnemies.clear();

        if (round >= 0) { // Early rush
            bc_VecUnit* allUnits = bc_GameController_units(gc);
            int allLen = bc_VecUnit_len(allUnits);
            for (int i = 0; i < allLen; ++i)
            {
                bc_Unit* unit = bc_VecUnit_index(allUnits, i);
                if (bc_Unit_team(unit) == enemyTeam && bc_Unit_unit_type(unit) != Worker)
                {
                    bc_Location* loc = bc_Unit_location(unit);
                    if (!bc_Location_is_in_garrison(loc) &&
                        !bc_Location_is_in_space(loc) &&
                        bc_Location_is_on_planet(loc, myPlanet))
                    {
                        bc_MapLocation* mapLoc = bc_Location_map_location(loc);

                        int x = bc_MapLocation_x_get(mapLoc);
                        int y = bc_MapLocation_y_get(mapLoc);
                        targetEnemies.push_back({x, y});

                        delete_bc_MapLocation(mapLoc);
                    }
                    delete_bc_Location(loc);
                }
                delete_bc_Unit(unit);
            }
            delete_bc_VecUnit(allUnits);

            if (!targetEnemies.size())
            {
                // For now, let's add an arbitrary point to target.
                // Let's use initial enemy unit locations.
                if (myPlanet == Earth)
                {
                    targetEnemies = initialEnemies;
                }
                else
                {
              //      targetEnemies.push_back({myPlanetR-1, myPlanetC-1});
                }
            }
        }

        if (targetEnemies.size()) Voronoi::findClosest(targetEnemies, myPlanetR, myPlanetC);
 		// Also, if targetEnemies.size(), we don't want units to do their normal walking stuff
        compHealth(gc);
        if (myPlanet == Earth && ((round >= lastRocket + 40 && round >= 250) || (round >= 350 && round >= lastRocket + 25) || (round >= goToMarsRound-20) || enemyIsDead) && !savingForFactory && nWorkers)
        {
            // we should make a rocket
            // let's make sure we actually have enough factories
            // before we do anything (or it's super super urgent)
            int numberUnits = nWorkers + nRangers + nMages + nWorkers - nInGarrison;
            if (enemyIsDead && round <= goToMarsRound)
            {
                if (numberUnits >= 15 && numberUnits/8 > nRockets)
                {
                    savingForRocket = true;
                    pair<bc_Unit*, bc_Direction> bestLoc = factoryLocation(gc, units, len, Rocket, round);
                    bc_Unit* bestUnit = bestLoc.first;
                    bc_Direction bestDir = bestLoc.second;
                    if (bestDir != Center)
                    {
                        // we can build a rocket!
                        // yaaay
                        printf("Building a rocket once all opponents are dead\n");
                        uint16_t id = bc_Unit_id(bestUnit);
                        createBlueprint(gc, bestUnit, id, 3, bestDir, Rocket);
                        savingForRocket = false;
                        lastRocket = round;
                    }
                }
                else savingForRocket = false;
            }
            else if (round >= goToMarsRound-20)
            {
                if (numberUnits/10 > nRockets)
                {
                    // Build a new rocket
                    savingForRocket = true;
                    pair<bc_Unit*, bc_Direction> bestLoc = factoryLocation(gc, units, len, Rocket, round);
                    bc_Unit* bestUnit = bestLoc.first;
                    bc_Direction bestDir = bestLoc.second;
                    if (bestDir != Center)
                    {
                        // we can build a rocket!
                        // yaaay
                        printf("Building a rocket after round 700\n");
                        uint16_t id = bc_Unit_id(bestUnit);
                        createBlueprint(gc, bestUnit, id, 3, bestDir, Rocket);
                        savingForRocket = false;
                        lastRocket = round;
                    }
                }
                else savingForRocket = false;
            }
            else
            {
                savingForRocket = true;
                pair<bc_Unit*, bc_Direction> bestLoc = factoryLocation(gc, units, len, Rocket, round);
                bc_Unit* bestUnit = bestLoc.first;
                bc_Direction bestDir = bestLoc.second;
                //printf("trying %d\n", bc_GameController_karbonite(gc));
                if (bestDir != Center)
                {
                    // we can build a rocket!
                    // yaaay
                    printf("Building a rocket\n");
                    uint16_t id = bc_Unit_id(bestUnit);
                    createBlueprint(gc, bestUnit, id, 3, bestDir, Rocket);
                    savingForRocket = false;
                    lastRocket = round;
                }
            }
        }
        for (int i = 0; i < len; i++)
        {
        	if (myPlanet == Mars) weExisted = true;
            bc_Unit *unit = bc_VecUnit_index(units, i);
            bc_UnitType unitType = bc_Unit_unit_type(unit);
            uint16_t id = bc_Unit_id(unit);
            bc_Location* loc = bc_Unit_location(unit);
            if (myPlanet == Earth) tryToLoadIntoRocket(gc, unit, loc, (round >= goToMarsRound || enemyIsDead));
            if (unitType == Worker)
            {
                if (myPlanet == Mars) goto loopCleanup;
                if (assignedStructure.find(id) != assignedStructure.end() &&
                    bc_Location_is_on_map(loc))
                {

                    bc_MapLocation* mapLoc = bc_Location_map_location(loc);

                    uint16_t structureid = assignedStructure[id];
                    bc_Unit *structure = bc_GameController_unit(gc, structureid);

                    if (structure &&
                        (!bc_Unit_structure_is_built(structure)
                    #if USE_PERMANENTLY_ASSIGNED_WORKERS
                         || bc_Unit_health(structure) <= 250           
                    #endif
                         ))
                    {
                        // what if we're not adjacent to the structure?
                        // let's try to go to it
                        // also, go around workers
                        bc_Location* structLoc = bc_Unit_location(structure);
                        bc_MapLocation* structMapLoc = bc_Location_map_location(structLoc);
                        delete_bc_Location(structLoc);

                        if (!bc_MapLocation_is_adjacent_to(mapLoc, structMapLoc))
                        {
                            // let's BFS to find the structure
                            // note: we have to navigate around factories and rockets
                            // (and of course terrain) but not much else
                            // oh god how many BFSs are there now
                            if (bc_GameController_is_move_ready(gc, id))
                            {
                                bool seenLoc[60][60];
                                bc_Direction prev[60][60];

                                for (int x = 0; x < 60; ++x) for (int y = 0; y < 60; ++y) seenLoc[x][y] = 0;

                                queue<pair<int,int>> Q;
                                int ox = bc_MapLocation_x_get(mapLoc);
                                int oy = bc_MapLocation_y_get(mapLoc);
                                int tx = bc_MapLocation_x_get(structMapLoc);
                                int ty = bc_MapLocation_y_get(structMapLoc);

                                int x = ox, y = oy;

                                delete_bc_MapLocation(structMapLoc);

                                bc_MapLocation* tmpMapLoc = new_bc_MapLocation(Earth, x, y);

                                Q.push({x, y});
                                seenLoc[x][y] = 1;
                                while (Q.size())
                                {
                                    tie(x, y) = Q.front(); Q.pop();

                                    if (x == tx && y == ty)
                                    {
                                        bc_Direction dir;
                                        while (x ^ ox || y ^ oy)
                                        {
                                            dir = prev[x][y];
                                            x -= bc_Direction_dx(dir);
                                            y -= bc_Direction_dy(dir);
                                        }

                                        if (bc_GameController_can_move(gc, id, dir))
                                        {
                                            bc_GameController_move_robot(gc, id, dir);
                                        } else printf("Failed to navigate towards structure\n");

                                        break;
                                    }

                                    if (earth.earth[x][y]) continue;

                                    bc_MapLocation_x_set(tmpMapLoc, x); bc_MapLocation_y_set(tmpMapLoc, y);
                                    bc_Unit* unitAtLoc = bc_GameController_sense_unit_at_location(gc, tmpMapLoc);
                                    if (unitAtLoc)
                                    {
                                        bc_UnitType typeAtLoc = bc_Unit_unit_type(unitAtLoc);
                                        delete_bc_Unit(unitAtLoc);

                                        if ((x ^ ox || y ^ oy))
                                        {
                                            continue;
                                        }
                                    }

                                    for (int d = 0; d < 8; ++d)
                                    {
                                        int dx = bc_Direction_dx((bc_Direction)d);
                                        int dy = bc_Direction_dy((bc_Direction)d);
                                        if (x+dx < 0 || x+dx >= earth.c ||
                                            y+dy < 0 || y+dy >= earth.r)
                                        {
                                            continue;
                                        }

                                        if (seenLoc[x+dx][y+dy])
                                        {
                                            continue;
                                        }

                                        seenLoc[x+dx][y+dy] = 1;
                                        prev[x+dx][y+dy] = (bc_Direction)d;
                                        Q.push({x+dx, y+dy});
                                    }
                                }

                                delete_bc_MapLocation(tmpMapLoc);
                            }
                        }

                        // build the structure if we can
                        if (bc_GameController_can_build(gc, id, structureid))
                        {
                            bc_GameController_build(gc, id, structureid);
                        }

                        // repair the structure if we can
                        if (bc_GameController_can_repair(gc, id, structureid))
                        {
                            bc_GameController_repair(gc, id, structureid);
                        }

                        delete_bc_Unit(structure);
                        delete_bc_MapLocation(mapLoc);
                        goto loopCleanup; // i'm sorry
                    }
                    else assignedStructure.erase(id);

                    // if this unit is not a permanent assignee:
                    // this unit no longer has an assigned structure
                    #if USE_PERMANENTLY_ASSIGNED_WORKERS
                    if (permanentAssignedStructure.find(id) == permanentAssignedStructure.end())
                    {
                    #endif
                        assignedStructure.erase(id);
                    #if USE_PERMANENTLY_ASSIGNED_WORKERS
                    }
                    else
                    {
                        // This unit needs to figure out
                        // what's going on.
                        // It won't go anywhere.

                        if (!structure)
                        {
                            // Rip, the structure died
                            // rebuild
                            int req = reqAssignees[structureid];
                            reqAssignees.erase(structureid);
                            assignedStructure.erase(id);
                            permanentAssignedStructure.erase(id);
                            createBlueprint(gc, unit, id, req,
                                            permanentAssignedDirection[id], permanentAssignedType[id]);
                        }

                        // We know it must have been completely built
                        // by this point.
                        // If we need to perform repairs,
                        // that'll be handled the next time
                        // that we look at this unit.

                        if (structure) delete_bc_Unit(structure);
                        delete_bc_MapLocation(mapLoc);

                        goto loopCleanup;
                    }
                    #endif

                    delete_bc_Unit(structure);
                    delete_bc_MapLocation(mapLoc);
                }


            }
            else
            {
                bool goingToRocket = false;

                if (/*(round >= goToMarsRound || enemyIsDead) &&*/ myPlanet == Earth)
                {
                    // PANIC PANIC PANIC
                    // We've got to get to Mars.
                    // First check that the current unit actually can do something.
                    if (bc_Location_is_on_map(loc))
                    {
                        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                        int x = bc_MapLocation_x_get(mapLoc);
                        int y = bc_MapLocation_y_get(mapLoc);

                        delete_bc_MapLocation(mapLoc);

                        // Check that the distance from the nearest factory is close enough.
                        // Note: this is approximate, it doesn't really matter
                        if (distToRocket[x][y] <= 1e9 && directionFromRocket[x][y] != (bc_Direction)8)
                        // note: doesn't handle healers at the moment
                        {
                            // Let's move
                            bc_Direction dir = bc_Direction_opposite(directionFromRocket[x][y]);

                            if (bc_GameController_can_move(gc, id, dir) &&
                                bc_GameController_is_move_ready(gc, id))
                            {
                                bc_GameController_move_robot(gc, id, dir);
                            }

                            // This flag is basically used so that
                            // our units know not to deviate from the path
                            // if they're going to the rocket
                            // (i.e. so they don't waste their movement heat on walking.)
                            goingToRocket = true;
                        }
                    }
                }

                // Rocket launch code (for testing, doesn't actually take anything into account)
                // Notice that, at present, because the VecUnits isn't actually sorted,
                // this might launch before every adjacent worker is in.
                if (unitType == Rocket)
                {
                    if (!bc_Unit_structure_is_built(unit) || myPlanet == Mars) goto loopCleanup;
                    bc_VecUnitID *garrisonUnits = bc_Unit_structure_garrison(unit);
                    int len = bc_VecUnitID_len(garrisonUnits);
                    delete_bc_VecUnitID(garrisonUnits);
                    if ((round == 749 || bc_Unit_health(unit) <= 140) && len != bc_Unit_structure_max_capacity(unit))
                    {
                        mars.updateKarboniteAmount(gc);
                        pair<int, int> landingLocPair = mars.optimalsquare(round);
                        bc_MapLocation* landingLoc = new_bc_MapLocation(Mars, landingLocPair.first, landingLocPair.second);
                        printf("%d %d\n", landingLocPair.first, landingLocPair.second);
                        if (bc_GameController_can_launch_rocket(gc, id, landingLoc)) //and now lets take off
                        {
                            printf("Launching. . .  %d\n", len);
                            bc_GameController_launch_rocket(gc, id, landingLoc);
                        }
                        else printf("Launch FAILED\n");
                        delete_bc_MapLocation(landingLoc);
                    }
                    else if (len == bc_Unit_structure_max_capacity(unit) && bc_Location_is_on_map(loc))
                    {
                    	bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                    	int x = bc_MapLocation_x_get(mapLoc);
                    	int y = bc_MapLocation_y_get(mapLoc);
                    	fullRockets.emplace(x, y);
                    	delete_bc_MapLocation(mapLoc);
                    }
                }
                else if (unitType == Factory)
                {
                    if (myPlanet != Earth) goto loopCleanup;


                    // try to unload
                    bool didUnload = false;
                    for (int j = 0; j < 8; ++j)
                    {
                        if (bc_GameController_can_unload(gc, id, (bc_Direction)j))
                        {
                        	didUnload = true;
                        	printf("Unloaded from factory\n");
                            bc_GameController_unload(gc, id, (bc_Direction)j);
                        }
                    }
                    if (!didUnload)
                    {
                    // check for surrounding squares
                    bc_VecUnitID* myUnits = bc_Unit_structure_garrison(unit);
                    int garrisonlen = bc_VecUnitID_len(myUnits);
                    delete_bc_VecUnitID(myUnits);
                    bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                    int x = bc_MapLocation_x_get(mapLoc);
                    int y = bc_MapLocation_y_get(mapLoc);
                    delete_bc_MapLocation(mapLoc);
                    if (garrisonlen && nfreeWorkers < 2)
                    {
                    	// we should disintegrate
                    	for (int l = 0; l < 8; l++)
                    	{
                    		int i = x + bc_Direction_dx((bc_Direction)l);
                    		int j = y + bc_Direction_dy((bc_Direction)l);
                    		mapLoc = new_bc_MapLocation(dealWithRangers.myPlanet, i, j);

                    		if (bc_GameController_has_unit_at_location(gc, mapLoc))
                    		{
                    			bc_Unit* killUnit = bc_GameController_sense_unit_at_location(gc, mapLoc);
                    			if (bc_Unit_team(killUnit) == dealWithRangers.myTeam)
                    			{
                    				if (bc_Unit_unit_type(killUnit) != Factory && bc_Unit_unit_type(killUnit) != Rocket)
                    				{
                    					uint16_t id = bc_Unit_id(killUnit);
        								bc_GameController_disintegrate_unit(gc, id);
        								printf("Disintegrated unit to unload worker\n");
        								delete_bc_MapLocation(mapLoc);
                                        delete_bc_Unit(killUnit);
                                        break;
                    				}
                    			}
                    			delete_bc_Unit(killUnit);
                    		}
                    		delete_bc_MapLocation(mapLoc);
                    	}
                    	// try to unload again
                    	for (int j = 0; j < 8; ++j)
                    	{
                        	if (bc_GameController_can_unload(gc, id, (bc_Direction)j))
                        	{
                        		printf("Unloaded worker from factory\n");
                            	bc_GameController_unload(gc, id, (bc_Direction)j);
                        	}	
                    	}
                    }
                	}
                    // Check around the structure to ensure that
                    // at least one unit is permanently assigned to it.
                    // If none are, arbitrarily assign one.
                    #if USE_PERMANENTLY_ASSIGNED_WORKERS
                        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                        bc_Direction dir = Center;
                        uint16_t adjid = 0;
                        bool hasPermanentAssignee = false;
                        for (int d = 0; d < 8; ++d)
                        {
                            bc_MapLocation* newLoc = bc_MapLocation_add(mapLoc, (bc_Direction)d);
                            bc_Unit* adjUnit = bc_GameController_sense_unit_at_location(gc, newLoc);
                            if (!adjUnit)
                            {
                                delete_bc_MapLocation(newLoc);
                                continue;
                            }

                            // if this unit is not assigned to this structure
                            uint16_t tadjid = bc_Unit_id(adjUnit);
                            if (assignedStructure.find(tadjid) == assignedStructure.end() ||
                                assignedStructure[tadjid] != id)
                            {
                                delete_bc_MapLocation(newLoc);
                                delete_bc_Unit(adjUnit);
                                continue;
                            }

                            uint16_t adjid = tadjid;
                            dir = (bc_Direction)d;
                            if (permanentAssignedStructure.find(adjid) != permanentAssignedStructure.end())
                            {
                                // this neighbour is permanently assigned to this structure
                                hasPermanentAssignee = true; 

                                delete_bc_MapLocation(newLoc);
                                delete_bc_Unit(adjUnit);

                                break;
                            }

                            delete_bc_MapLocation(newLoc);
                            delete_bc_Unit(adjUnit);
                        }

                        if (!hasPermanentAssignee)
                        {
                            // We can arbitrarily assign a permanent worker
                            // to this structure.
                            // If dir is still Center,
                            // there are no adjacent assigned workers...
                            // rip.
                            // Otherwise we just assign one of them.
                            if (dir != Center)
                            {
                                assignedStructure[adjid] = id;
                                permanentAssignedStructure[adjid] = id;
                                permanentAssignedType[adjid] = Factory;
                                permanentAssignedDirection[adjid] = bc_Direction_opposite(dir);
                            }
                            else
                            {
                                // We have no adjacent assigned workers.
                                // Create another one and assign it to us.

                                if (!bc_Unit_structure_is_built(unit))
                                {
                                    // All adjacent workers died before this factory was finished.
                                    // Not really much we can do now, except pull in a worker
                                    // from somewhere else... which can be implemented later.
                                    goto loopCleanup;
                                }

                                if (bc_GameController_can_produce_robot(gc, id, Worker))
                                {
                                    bc_GameController_produce_robot(gc, id, Worker);
                                }

                                for (int j = 0; j < 8; ++j)
                                {
                                    if (bc_GameController_can_unload(gc, id, (bc_Direction)j))
                                    {
                                        bc_GameController_unload(gc, id, (bc_Direction)j);

                                        // if the unloaded unit was a worker:
                                        // assign it to this factory
                                        bc_MapLocation* newLoc = bc_MapLocation_add(mapLoc, (bc_Direction)j);
                                        bc_Unit* newUnit = bc_GameController_sense_unit_at_location(gc, newLoc);

                                        if (bc_Unit_unit_type(newUnit) == Worker)
                                        {
                                            uint16_t newid = bc_Unit_id(newUnit);

                                            assignedStructure[newid] = id;
                                            printf("Assigning\n");
                                            permanentAssignedStructure[newid] = id;
                                            permanentAssignedType[newid] = Factory;
                                            permanentAssignedDirection[newid] = bc_Direction_opposite((bc_Direction)j);

                                            delete_bc_MapLocation(newLoc);
                                            delete_bc_Unit(newUnit);

                                            break;
                                        }

                                        delete_bc_MapLocation(newLoc);
                                        delete_bc_Unit(newUnit);
                                    }
                                }

                                delete_bc_MapLocation(mapLoc);

                                goto loopCleanup;
                            }
                        }

                        delete_bc_MapLocation(mapLoc);
                    #endif

                    // Let's build some stuff.
                    // Why not.
                    if (!bc_Unit_structure_is_built(unit)) goto loopCleanup;

                    // Choose proportions to make it work well

                    // healers, knights : mages : rangers
                    vector<int> ratioKMR = {8, 0, 0, 20}; // {7, 0, 3, 20}
          			//if (round < 350) ratioKMR = {1, 0, 0, 2};
                    if (round < 180) ratioKMR = {3, 4, 0, 6};
                    else if (round < 550) ratioKMR = {1, 0, 0, 2}; // {5, 0, 1, 9}

                    bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                    int x = bc_MapLocation_x_get(mapLoc);
                    int y = bc_MapLocation_y_get(mapLoc);
                    delete_bc_MapLocation(mapLoc);

                    int mnDist = getRatioDistance({nHealers, nKnights + 1, nMages, nRangers}, ratioKMR);
                    bc_UnitType type = Knight;

                    if (getRatioDistance({nHealers, nKnights, nMages + 1, nRangers}, ratioKMR) <= mnDist)
                    {
                        mnDist = getRatioDistance({nHealers, nKnights, nMages + 1, nRangers}, ratioKMR);
                        type = Mage;
                    }

                    if (getRatioDistance({nHealers + 1, nKnights, nMages, nRangers}, ratioKMR) <= mnDist)
                    {
                        mnDist = getRatioDistance({nHealers + 1, nKnights, nMages, nRangers}, ratioKMR);
                        type = Healer;
                    }

                    if (getRatioDistance({nHealers, nKnights, nMages, nRangers + 1}, ratioKMR) <= mnDist)
                    {
                        mnDist = getRatioDistance({nHealers, nKnights, nMages, nRangers + 1}, ratioKMR);
                        type = Ranger;
                    }


                    if (nWorkers < 2) type = Worker;
                    if (!nWorkers || ((!savingForRocket || bc_GameController_karbonite(gc) > bc_UnitType_blueprint_cost(Rocket)) && (!savingForFactory || bc_GameController_karbonite(gc) > bc_UnitType_blueprint_cost(Factory))))
                    {
                        if (bc_GameController_can_produce_robot(gc, id, type))
                        {
                            bc_GameController_produce_robot(gc, id, type);

                            if (type == Ranger) nRangers++;
                            if (type == Knight) nKnights++;
                            if (type == Mage) nMages++;
                            if (type == Healer) nHealers++;
                        }
                    }
                }
                else
                {
                    if (unitType == Knight)
                    {
                        // if we are in a garrison or space:
                        // wow, not much you can do now
                        if (bc_Location_is_in_garrison(loc) ||
                            bc_Location_is_in_space(loc)) goto loopCleanup;

                        if (!targetEnemies.size())
                        {
                            // The knight can do its normal enemy stuff
                            bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                            bc_MapLocation* nearestEnemy;
                            bc_Direction dir;
                            tie(nearestEnemy, dir) = findNearestEnemy(gc, currTeam, map, mapLoc, 50, false);

                            if (!goingToRocket)
                            {
                                if (!nearestEnemy)
                                {
                                    // Let's just not move.
                                    dir = Center;
                                }

                                if (nearestEnemy && bc_MapLocation_is_adjacent_to(mapLoc, nearestEnemy))
                                {
                                    // The knight is adjacent to the nearest enemy.
                                    // If it's diagonally adjacent,
                                    // we'll move in such a way as to make it
                                    // vertically or horizontally adjacent.

                                    // NOTE: obsolete as knights now have
                                    // 8-dir attack

                                    /*
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
                                    */
                                    if (dir != Center &&
                                        bc_GameController_can_move(gc, id, dir) &&
                                        bc_GameController_is_move_ready(gc, id))
                                    {
                                        bc_GameController_move_robot(gc, id, dir);
                                    }
                                }
                                else
                                {
                                    if (dir != Center &&
                                        bc_GameController_can_move(gc, id, dir) &&
                                        bc_GameController_is_move_ready(gc, id))
                                    {
                                        bc_GameController_move_robot(gc, id, dir);
                                    }
                                }
                            }

                            if (nearestEnemy)
                            {
                                uint16_t enemyid = dealWithRangers.enemy(
                                    bc_MapLocation_x_get(nearestEnemy),
                                    bc_MapLocation_y_get(nearestEnemy)
                                );

                                if (enemyid)
                                {
                                    enemyid--;
                                    if (bc_GameController_can_attack(gc, id, enemyid) &&
                                        bc_GameController_is_attack_ready(gc, id))
                                    {
                                        bc_GameController_attack(gc, id, enemyid);
                                    }
                                }

                                delete_bc_MapLocation(nearestEnemy);
                            }
                            delete_bc_MapLocation(mapLoc);
                        }
                        else
                        {
                            // if we can attack the nearest enemy:
                            // do it

                            bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                            int x = bc_MapLocation_x_get(mapLoc);
                            int y = bc_MapLocation_y_get(mapLoc);
                            delete_bc_MapLocation(mapLoc);

                            pair<int, int> nearestEnemyLoc = Voronoi::closestEnemy[x][y];

                            uint16_t enemyid = dealWithRangers.enemy(nearestEnemyLoc.first, nearestEnemyLoc.second);

                            if (enemyid)
                            {
                                enemyid--;
                                if (bc_GameController_can_attack(gc, id, enemyid) &&
                                    bc_GameController_is_attack_ready(gc, id))
                                {
                                    bc_GameController_attack(gc, id, enemyid);
                                }
                            }
                        }
                        if (bc_GameController_is_javelin_ready(gc, id)) dealWithRangers.doJavelinAttack(gc, unit);
                    }
                    else if (unitType == Ranger)
                    {
                        if (!bc_Location_is_in_garrison(loc) && !bc_Location_is_in_space(loc))
                        {
                            uint16_t id = bc_Unit_id(unit);
                            if (currSnipers.find(id) == currSnipers.end())
                            {
                                dealWithRangers.findNearestEnemy(gc, unit, !goingToRocket && !targetEnemies.size());
                            }
                        }
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

                        dealWithRangers.findNearestEnemy(gc, unit, !goingToRocket && !targetEnemies.size());
                        
                    }
                    else if (unitType == Healer)
                    {
                    	if (bc_Location_is_in_garrison(loc) ||
                            bc_Location_is_in_space(loc)) goto loopCleanup;
                        healerHeal(gc, unit, !goingToRocket && !targetEnemies.size());
                    }
                }
            }

            loopCleanup:
            if (unit)
            {
                // Note: don't delete the unit yet,
                // we need it later
                // delete_bc_Unit(unit);
                delete_bc_Location(loc);
            }
        }


        // We want our units to swarm in towards the nearest enemy.
        // We split them into two categories:
        // units which are too close (by Euclidean distance)
        // (and are not knights, because that'd be a bad idea to handle)
        // and units which are far enough away.
        // If units are knights, they're always far enough away.

        // tooClose contains pairs of {distance to nearest enemy, unit}
        if (targetEnemies.size())
        {
            #if USE_SNIPE
            rangersByDistance.clear();
            #endif

            // Union find the blobs
            for (int i = 0; i < 60; ++i) for (int j = 0; j < 60; ++j)
            {
                parentUnitLocation[i][j] = {i, j};
                minEuclideanDist[i][j] = maxEuclideanDist[i][j] = Voronoi::disToClosestEnemy[i][j];
                sizeOfBlob[i][j] = 1;
            }

            for (int i = 0; i < len; ++i)
            {
                bc_Unit* unit = bc_VecUnit_index(units, i);
                bc_UnitType type = bc_Unit_unit_type(unit);

                if (type != Worker)
                {
                    bc_Location* loc = bc_Unit_location(unit);
                    if (!bc_Location_is_in_garrison(loc) &&
                        !bc_Location_is_in_space(loc) &&
                        bc_Location_is_on_planet(loc, myPlanet))
                    {
                        bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                        int x = bc_MapLocation_x_get(mapLoc);
                        int y = bc_MapLocation_y_get(mapLoc);

                        pair<int, int> currParent = findParent(x, y);

                        // Union this with every adjacent unit
                        for (int d = 0; d < 8; ++d)
                        {
                            int dx = bc_Direction_dx((bc_Direction)d);
                            int dy = bc_Direction_dy((bc_Direction)d);

                            bc_MapLocation* newLoc = bc_MapLocation_add(mapLoc, (bc_Direction)d);
                            bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, newLoc);
                            if (unit)
                            {
                                if (bc_Unit_team(unit) == currTeam)
                                {
                                    pair<int, int> newParent = findParent(x+dx, y+dy);
                                    if (newParent != currParent)
                                    {
                                        if (sizeOfBlob[newParent.first][newParent.second] >
                                            sizeOfBlob[currParent.first][currParent.second])
                                        {
                                            swap(currParent, newParent);
                                        }
                                        sizeOfBlob[currParent.first][currParent.second] +=
                                            sizeOfBlob[newParent.first][newParent.second];
                                        parentUnitLocation[newParent.first][newParent.second] = currParent;
                                        minEuclideanDist[currParent.first][currParent.second] = 
                                            min(minEuclideanDist[currParent.first][currParent.second],
                                                minEuclideanDist[newParent.first][newParent.second]);
                                        maxEuclideanDist[currParent.first][currParent.second] =
                                            max(maxEuclideanDist[currParent.first][currParent.second],
                                                maxEuclideanDist[newParent.first][newParent.second]);
                                    }
                                }
                                delete_bc_Unit(unit);
                            }
                            delete_bc_MapLocation(newLoc);
                        }
                        delete_bc_MapLocation(mapLoc);
                    }
                    delete_bc_Location(loc);
                }
            }

            tooClose.clear();
            int totalRangers = 0;
            for (int i = 0; i < len; ++i)
            {
                bc_Unit* unit = bc_VecUnit_index(units, i);
                bc_UnitType type = bc_Unit_unit_type(unit);

                if (type != Knight && type != Worker)
                {
                    int attackRange = bc_Unit_attack_range(unit);
                    if (type == Healer) attackRange = HealerRange;
                    bc_Location* loc = bc_Unit_location(unit);
                    if (!bc_Location_is_in_garrison(loc) &&
                        !bc_Location_is_in_space(loc) &&
                        bc_Location_is_on_planet(loc, myPlanet))
                    {
                        bc_MapLocation* mapLoc = bc_Location_map_location(loc);

                        int x = bc_MapLocation_x_get(mapLoc);
                        int y = bc_MapLocation_y_get(mapLoc);

                        #if USE_SNIPE
                        if (type == Ranger)
                        {
                            // this is a ranger
                            // and thus a candidate to shoot some enemies
                            uint16_t id = bc_Unit_id(unit);
                            if (currSnipers.find(id) == currSnipers.end() &&
                                bc_GameController_is_begin_snipe_ready(gc, id))
                            {
                                rangersByDistance.push_back({Voronoi::disToClosestEnemy[x][y], {x, y}});
                            }

                            totalRangers++;
                        }
                        #endif

                        pair<int, int> currParent = findParent(x, y);
                        int sizeOfCurrBlob = sizeOfBlob[currParent.first][currParent.second];
                        int widthOfCurrBlob = maxEuclideanDist[currParent.first][currParent.second]
                                            - minEuclideanDist[currParent.first][currParent.second];

                        int TOOCLOSENESS_FACTOR = 5;
                        if (widthOfCurrBlob < 50 || sizeOfCurrBlob < 10) TOOCLOSENESS_FACTOR = 0;

                        if (Voronoi::disToClosestEnemy[x][y] < attackRange - TOOCLOSENESS_FACTOR)
                        {
                            tooClose.push_back({Voronoi::disToClosestEnemy[x][y], unit});
                        }

                        delete_bc_MapLocation(mapLoc);
                    }
                    delete_bc_Location(loc);
                }
            }

            sort(tooClose.begin(), tooClose.end());
            for (int i = tooClose.size(); i--; )
            {
                // Process the units by euclidean distance from enemy decreasing
                // A simple heuristic to run away is to just move to a nearby square
                // that does not increase euclidean distance,
                // or not move if that square doesn't exist.
                int currDist = tooClose[i].first;
                bc_Unit* unit = tooClose[i].second;
                uint16_t id = bc_Unit_id(unit);

                if (!bc_GameController_is_move_ready(gc, id)) continue;

                bc_Location* loc = bc_Unit_location(unit);
                bc_MapLocation* mapLoc = bc_Location_map_location(loc);
                int x = bc_MapLocation_x_get(mapLoc);
                int y = bc_MapLocation_y_get(mapLoc);

                bc_Direction bestDir = Center;
                for (int d = 0; d < 8; ++d)
                {
                    if (!bc_GameController_can_move(gc, id, (bc_Direction)d)) continue;

                    int dx = bc_Direction_dx((bc_Direction)d);
                    int dy = bc_Direction_dy((bc_Direction)d);

                    if (Voronoi::disToClosestEnemy[x+dx][y+dy] >= currDist)
                    {
                        bestDir = (bc_Direction)d;
                        currDist = Voronoi::disToClosestEnemy[x+dx][y+dy];
                    }
                }

                delete_bc_MapLocation(mapLoc);
                delete_bc_Location(loc);

                if (bestDir == Center) continue;
                bc_GameController_move_robot(gc, id, bestDir);
            }

            // clean up
            for (int i = 0; i < len; ++i) delete_bc_Unit(bc_VecUnit_index(units, i));

            #if USE_SNIPE
                // only snipe if round is a multiple of 8,
                // which 'syncs' our snipes
                if (round % 8 == 0)
                {
                    // sort the rangers by distance to the closest enemy,
                    // so we can choose which ones to use for sniping
                    sort(rangersByDistance.begin(), rangersByDistance.end());

                    structuresToSnipe.clear();
                    unitsToSnipe.clear();

                    bc_VecUnit* allUnits = bc_GameController_units(gc);
                    int alen = bc_VecUnit_len(allUnits);
                    for (int i = 0; i < alen; ++i)
                    {
                        bc_Unit* unit = bc_VecUnit_index(allUnits, i);
                        if (bc_Unit_team(unit) == enemyTeam &&
                            (bc_Unit_unit_type(unit) == Factory ||
                             bc_Unit_unit_type(unit) == Rocket))
                        {
                            bc_Location* loc = bc_Unit_location(unit);
                            bc_MapLocation* mapLoc = bc_Location_map_location(loc);

                            int x = bc_MapLocation_x_get(mapLoc);
                            int y = bc_MapLocation_y_get(mapLoc);

                            int unitHealth = bc_Unit_health(unit);

                            structuresToSnipe.push_back({unitHealth, {x, y}});

                            delete_bc_Location(loc);
                            delete_bc_MapLocation(mapLoc);
                        }
                        else if (bc_Unit_team(unit) == enemyTeam &&
                                 bc_Unit_unit_type(unit) == Worker)
                        {
                            bc_Location* loc = bc_Unit_location(unit);
                            bc_MapLocation* mapLoc = bc_Location_map_location(loc);

                            int x = bc_MapLocation_x_get(mapLoc);
                            int y = bc_MapLocation_y_get(mapLoc);

                            int unitHealth = bc_Unit_health(unit);

                            // count the number of adjacent enemy units
                            // or walls
                            int nAdj = 0;
                            for (int d = 0; d < 8; ++d)
                            {
                                int dx = bc_Direction_dx((bc_Direction)d);
                                int dy = bc_Direction_dy((bc_Direction)d);

                                if (x+dx < 0 || x+dx >= myPlanetC || y+dy < 0 || y+dy >= myPlanetR)
                                {
                                    nAdj++;
                                    continue;
                                }

                                if ((myPlanet == Earth && earth.earth[x+dx][y+dy]) ||
                                    (myPlanet == Mars && mars.mars[x+dx][y+dy]))
                                {
                                    nAdj++;
                                    continue;
                                }

                                uint16_t enemyid = dealWithRangers.enemy(x+dx, y+dy);
                                if (enemyid) nAdj++;
                            }

                            unitsToSnipe.push_back({nAdj, {unitHealth, {x, y}}});

                            delete_bc_Location(loc);
                            delete_bc_MapLocation(mapLoc);
                        }
                        delete_bc_Unit(unit);
                    }
                    delete_bc_VecUnit(allUnits);

                    // for every enemy structure we can see:
                    // try and shoot it
                    for (int i = 0; structuresToSnipe.size() && rangersByDistance.size() && i < totalRangers / 2; ++i)
                    {
                        int x, y;
                        tie(x, y) = rangersByDistance.back().second;
                        rangersByDistance.pop_back();

                        bc_MapLocation* mapLoc = new_bc_MapLocation(myPlanet, x, y);
                        bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, mapLoc);
                        uint16_t id = bc_Unit_id(unit);

                        int ex, ey;
                        tie(ex, ey) = structuresToSnipe.back().second;
                        bc_MapLocation* enemyMapLoc = new_bc_MapLocation(myPlanet, ex, ey);

                        if (bc_GameController_can_begin_snipe(gc, id, enemyMapLoc))
                        {
                            bc_GameController_begin_snipe(gc, id, enemyMapLoc);
                            currSnipers.insert(id);
                            sniperEndTimes.push({id, round + 5});

                            structuresToSnipe.back().first -= 30;
                            if (structuresToSnipe.back().first <= 0) structuresToSnipe.pop_back();
                            printf("SN1P3\n");
                        }

                        delete_bc_MapLocation(mapLoc);
                        delete_bc_Unit(unit);
                        delete_bc_MapLocation(enemyMapLoc);
                    }

                    sort(unitsToSnipe.begin(), unitsToSnipe.end());
                    for (int i = 0; unitsToSnipe.size() && rangersByDistance.size() && i < totalRangers / 2; ++i)
                    {
                        int x, y;
                        tie(x, y) = rangersByDistance.back().second;
                        rangersByDistance.pop_back();

                        bc_MapLocation* mapLoc = new_bc_MapLocation(myPlanet, x, y);
                        bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, mapLoc);
                        uint16_t id = bc_Unit_id(unit);

                        int ex, ey;
                        tie(ex, ey) = unitsToSnipe.back().second.second;
                        bc_MapLocation* enemyMapLoc = new_bc_MapLocation(myPlanet, ex, ey);

                        if (bc_GameController_can_begin_snipe(gc, id, enemyMapLoc))
                        {
                            bc_GameController_begin_snipe(gc, id, enemyMapLoc);
                            currSnipers.insert(id);
                            sniperEndTimes.push({id, round + 5});

                            unitsToSnipe.back().second.first -= 30;
                            if (unitsToSnipe.back().second.first <= 0) unitsToSnipe.pop_back();
                            printf("SN1P3\n");
                        }

                        delete_bc_MapLocation(mapLoc);
                        delete_bc_Unit(unit);
                        delete_bc_MapLocation(enemyMapLoc);
                    }
                }
            #endif

            // We need to BFS from enemies and thus process
            // units (obviously those that aren't too close, or are knights)
            // in order of BFS distance from the enemy.
            // When we move, we move to any adjacent square with minimum (BFS)
            // distance from the enemy that we can. We don't walk to a square
            // if it's too close to the enemy, though (unless we're a knight).
            // Hey would you look at that we have a list
            // of enemies to BFS from in targetEnemies. Great
            for (int i = 0; i < myPlanetC; ++i) for (int j = 0; j < myPlanetR; ++j) unitMovementSeen[i][j] = 0;

            // We don't actually need to initialise unitMovementDist, so we won't
            for (auto p : targetEnemies)
            {
                unitMovementBFSQueue.push(p);
                unitMovementSeen[p.first][p.second] = 1;
                unitMovementDist[p.first][p.second] = 0;
            }

            while (unitMovementBFSQueue.size())
            {
                int x, y;
                tie(x, y) = unitMovementBFSQueue.front(); unitMovementBFSQueue.pop();

                if ((myPlanet == Earth && earth.earth[x][y]) ||
                    (myPlanet == Mars && mars.mars[x][y]))
                {
                    continue;
                }

                bc_MapLocation* mapLoc = new_bc_MapLocation(myPlanet, x, y);
                if (bc_GameController_has_unit_at_location(gc, mapLoc))
                {
                    bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, mapLoc);
                    bc_UnitType type = bc_Unit_unit_type(unit);
                    uint16_t id = bc_Unit_id(unit);
                    int attackRange = bc_Unit_attack_range(unit);
                    if (type == Healer) attackRange = HealerRange;

                    pair<int, int> currParent = findParent(x, y);
                    int sizeOfCurrBlob = sizeOfBlob[currParent.first][currParent.second];
                    int widthOfCurrBlob = maxEuclideanDist[currParent.first][currParent.second]
                                        - minEuclideanDist[currParent.first][currParent.second];

                    int CLOSENESS_FACTOR = 2;
                    if (widthOfCurrBlob < 50 || sizeOfCurrBlob < 10) CLOSENESS_FACTOR = 0;
                    if (round <= 80) CLOSENESS_FACTOR = 5;

                    if ((Voronoi::disToClosestEnemy[x][y] >= attackRange - CLOSENESS_FACTOR || type == Knight) &&
                        type != Worker && bc_GameController_is_move_ready(gc, id) &&
                        currSnipers.find(id) == currSnipers.end())
                    {
                        vector<bc_Direction> bestDirs;
                        int bestDist = unitMovementDist[x][y];
                        for (int d = 0; d < 8; ++d)
                        {
                            if (!bc_GameController_can_move(gc, id, (bc_Direction)d)) continue;
                            
                            int dx = bc_Direction_dx((bc_Direction)d);
                            int dy = bc_Direction_dy((bc_Direction)d);
                            if (unitMovementSeen[x+dx][y+dy] &&
                                (type == Knight || Voronoi::disToClosestEnemy[x+dx][y+dy] >= attackRange - CLOSENESS_FACTOR))
                            {
                                if (unitMovementDist[x+dx][y+dy] < bestDist)
                                {
                                    bestDist = unitMovementDist[x+dx][y+dy];
                                    bestDirs.clear();
                                }
                                if (unitMovementDist[x+dx][y+dy] == bestDist)
                                {
                                    bestDirs.push_back((bc_Direction)d);
                                }
                            }
                        }
                        if (bestDirs.size())
                        {
                            // rand shuffle potential move directions
                            bc_GameController_move_robot(gc, id, bestDirs[rand()%bestDirs.size()]);
                        }
                    }
                    delete_bc_Unit(unit);
                }
                delete_bc_MapLocation(mapLoc);

                for (int d = 0; d < 8; ++d)
                {
                    int dx = bc_Direction_dx((bc_Direction)d);
                    int dy = bc_Direction_dy((bc_Direction)d);

                    if (x+dx < 0 || x+dx >= myPlanetC || y+dy < 0 || y+dy >= myPlanetR)
                    {
                        continue;
                    }

                    if (unitMovementSeen[x+dx][y+dy]) continue;
                    unitMovementSeen[x+dx][y+dy] = 1;
                    unitMovementDist[x+dx][y+dy] = unitMovementDist[x][y] + 1;
                    unitMovementBFSQueue.push({x+dx, y+dy});
                }
            }
        }

        // So if we have 5 full rockets, fly them all. Otherwise if a full rocket is withing 100 of an enemy .. we can't risk it
        // otherwise, do nothing
        if ((myPlanet == Earth && fullRockets.size() >= 5) || (round == 749 && fullRockets.size()) || (!shouldGoInGroup && fullRockets.size()))
        {
        	launchAllFullRockets(gc, round);
        }
        else if (myPlanet == Earth)
        {
        	toDelete.clear();
        	for (auto it = fullRockets.begin(); it != fullRockets.end(); ++it)
        	{
        		int x = it->first;
				int y = it->second;
				bc_MapLocation* mapLoc = new_bc_MapLocation(Earth, x, y);
				if (bc_GameController_has_unit_at_location(gc, mapLoc))
				{
					bc_Unit* unit = bc_GameController_sense_unit_at_location(gc, mapLoc);
					if (bc_Unit_unit_type(unit) == Rocket)
					{
						uint16_t id = bc_Unit_id(unit);
						if (Voronoi::disToClosestEnemy[x][y] <= 100)
						{
							printf("Enemy is too close to our rocket!\n");
							// we can be seen
							if (fullRockets.size() >= 3)
							{
								launchAllFullRockets(gc, round);
								break;
							}
							pair<int, int> landingLocPair = mars.optimalsquare(round);
					      	bc_MapLocation* landingLoc = new_bc_MapLocation(Mars, landingLocPair.first, landingLocPair.second);
    					    printf("%d %d\n", landingLocPair.first, landingLocPair.second);
        					if (bc_GameController_can_launch_rocket(gc, id, landingLoc)) //and now lets take off
			    		    {
    			    		    printf("Launching. . .\n");
        	    				bc_GameController_launch_rocket(gc, id, landingLoc);
					        }
    					    else printf("Launch FAILED\n");
        					delete_bc_MapLocation(landingLoc);
        					toDelete.emplace(x, y);
						}
					}
					else printf("No rocket here!\n");
					delete_bc_Unit(unit);
				}
				else printf("Somehow this rocket was killed (this is an issue)\n");
				delete_bc_MapLocation(mapLoc);
        	}
        	for (auto a : toDelete)
        	{
        		fullRockets.erase(a);
        	}
        }


        // For each structure which doesn't have enough workers:
        // duplicate workers around it to fulfil the quota
        // (we don't 'steal' workers that are passing by since
        //  that might stuff us up (edit: actually we do))
        // also make sure no units are at the locations
        // Note that factories which are completed
        // won't have any assigned.
        for (auto P : dirAssigned)
        {
            if (myPlanet == Mars) continue;
            uint16_t structureid; int numAssigned;
            tie(structureid, numAssigned) = P;
            bc_Unit *structure = bc_GameController_unit(gc, structureid);
            if (bc_Unit_structure_is_built(structure) && bc_Unit_health(structure) > bc_Unit_max_health(structure)-40)
            {
                delete_bc_Unit(structure);
                continue;
            }
            bc_Location *loc = bc_Unit_location(structure);
            bc_MapLocation *mapLoc = bc_Location_map_location(loc);

            if (numAssigned < reqAssignees[structureid])
            {
                // first let's BFS to try and see if we can steal us some workers
                // we'll only take workers that are reasonably close to our position
                // (and obviously only unassigned ones)
                // also, don't go over rockets or factories, except this one of course
                bool seenLoc[60][60];
                int dist[60][60];
                for (int i = 0; i < 60; ++i) for (int j = 0; j < 60; ++j) seenLoc[i][j] = 0;

                int ox = bc_MapLocation_x_get(mapLoc);
                int oy = bc_MapLocation_y_get(mapLoc);
                int x = ox, y = oy;

                bc_MapLocation* tmpMapLoc = new_bc_MapLocation(Earth, x, y);

                queue<pair<int,int>> Q;
                Q.push({x, y});
                seenLoc[x][y] = 1;
                dist[x][y] = 0;

                while (Q.size())
                {
                    tie(x, y) = Q.front(); Q.pop();
                    if (dist[x][y] > 5 && numAssigned) break;
                    if (numAssigned >= reqAssignees[structureid]) break;

                    if (earth.earth[x][y]) continue;

                    bc_MapLocation_x_set(tmpMapLoc, x); bc_MapLocation_y_set(tmpMapLoc, y);
                    bc_Unit* unitAtLoc = bc_GameController_sense_unit_at_location(gc, tmpMapLoc);
                    if (unitAtLoc)
                    {
                        bc_UnitType typeAtLoc = bc_Unit_unit_type(unitAtLoc);
                        if (typeAtLoc == Worker && bc_Unit_team(unitAtLoc) == currTeam)
                        {
                            uint16_t unitid = bc_Unit_id(unitAtLoc);
                            if (assignedStructure.find(unitid) == assignedStructure.end())
                            {
                                assignedStructure[unitid] = structureid;
                                numAssigned++;
                            }
                        }
                        else if (x ^ ox || y ^ oy)
                        {
                        	delete_bc_Unit(unitAtLoc);
                            continue;
                        }
                        delete_bc_Unit(unitAtLoc);
                    }

                    for (int d = 0; d < 8; ++d)
                    {
                        int dx = bc_Direction_dx((bc_Direction)d);
                        int dy = bc_Direction_dy((bc_Direction)d);
                        if (x+dx < 0 || x+dx >= earth.c ||
                            y+dy < 0 || y+dy >= earth.r)
                        {
                            continue;
                        }

                        if (seenLoc[x+dx][y+dy]) continue;

                        seenLoc[x+dx][y+dy] = 1;
                        dist[x+dx][y+dy] = dist[x][y] + 1;
                        Q.push({x+dx, y+dy});
                    }
                }

                if (numAssigned >= reqAssignees[structureid])
                {
                    delete_bc_Unit(structure);
                    delete_bc_Location(loc);
                    delete_bc_MapLocation(mapLoc);
                    continue;
                }

                // if we're _still_ not done...
                // let's duplicate some workers around us

                bc_MapLocation *structureAdj[8];
                uint16_t workerid[8];
                bool hasworker[8];
                for (int d = 0; d < 8; ++d)
                {
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


        
        if (myPlanet == Earth) mineKarboniteOnEarth(gc, nRangers + nMages + nKnights + nHealers, round); // mines karbonite on earth
        printf("time remaining: %d\n", bc_GameController_get_time_left_ms(gc));

        fflush(stdout);
        bc_GameController_next_turn(gc);
    }

    delete_bc_PlanetMap(map);
    delete_bc_GameController(gc);
}