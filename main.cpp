#include "bits/stdc++.h"
using namespace std;
#define mp make_pair
// oh god
#define this it
#include <bc.h>
#undef this

struct MarsSTRUCT //contains a more readable map of mars, as well as the code to find where to send a rocket
{
    int r, c;
    int mars[60][60]; // 0 = passable, 1 = unpassable
    int karbonite[60][60]; // number of karbonite at each square at some time
    int seen[60][60], upto, dis[60][60], comp[60][60], compsize[3000];
    void init(bc_GameController* gc)
    {
        bc_Planet bc_Planet_mars = Mars;
        bc_PlanetMap* map = bc_GameController_starting_map(gc, bc_Planet_mars);
        r = bc_PlanetMap_height_get(map);
        c = bc_PlanetMap_width_get(map);
        for (int i = 0; i < r; i++)
        {
            for (int j = 0; j < c; j++)
            {
                bc_MapLocation* loc = new_bc_MapLocation(bc_Planet_mars, i, j);
                mars[i][j] = !bc_PlanetMap_is_passable_terrain_at(map, loc);
          //      printf("%d ", mars[i][j]);
            }
        //    printf("\n");
        }
    }
    void findcomps(int x, int y, int k) // find components and their sizes
    {
        if (seen[x][y] == upto) return;
        seen[x][y] = upto;
        comp[x][y] = k;
        compsize[k]++;
        for (int i = max(x-1, 0); i <= min(x+1, r-1); i++)
        {
            for (int j = max(y-1, 0); j <= min(y+1, c-1); j++)
            {
                if (!mars[i][j]) findcomps(i, j, k);
          }
        }
    }
    int bfs(int x, int y, int k) // bfs for finding amount of karbonite within distance k
    {
        int ans = 0;
        queue<pair<int, int> > q;
        seen[x][y] = ++upto;
        dis[x][y] = 0;
        q.emplace(x, y);
        while (!q.empty())
        {
            int x = q.front().first;
            int y = q.front().second;
            q.pop();
            int d = dis[x][y];
            ans += karbonite[x][y];
            if (d == k) continue;
            for (int i = max(x-1, 0); i <= min(x+1, r-1); i++)
            {
                for (int j = max(y-1, 0); j <= min(y+1, c-1); j++)
                {
                    if (seen[i][j] != upto && !mars[i][j])
                    {
                        seen[i][j] = upto;
                        dis[i][j] = d+1;
                        q.emplace(i, j);
                    }
                }
            }
        }
        return ans;
    }
    int numneighbours(int x, int y) // returns number of valid neighbours
    {
        int ans = -1;
        for (int i = max(x-1, 0); i <= min(x+1, r-1); i++)
        {
            for (int j = max(y-1, 0); j <= min(y+1, c-1); j++)
            {
                if (!mars[i][j]) ans++;
            }
        }
        return ans;
    }

    pair<int, int> optimalsquare()
    {
        int A = 1; //constant, representing how much weighting the amount of karbonite should have
        int B = 1; //weighting of size of component
        int C = 1; //weighting of amount of neighbours
        // A (and possibly also B/C) could be dependent on total available karbonite/average comp size/whatever
        int K = 5; //dist to consider for karbonite
        upto++;
        for (int i = 0; i < r; i++)
        {
            for (int j = 0; j < c; j++) findcomps(i, j, i*c+j);
        }
        pair<pair<int, int>, pair<int, int> > best = mp(mp(0, 0), mp(0, 0));
        for (int i = 0; i < r; i++)
        {
            for (int j = 0; j < c; j++)
            {   
                int nearbykar = bfs(i, j, K);
                int compsz = compsize[comp[i][j]];
                int numnei = numneighbours(i, j);
                int rating = A*nearbykar + B*compsz + C*numnei;
                best = max(best, mp(mp(rating, rand()), mp(i, j))); //rand is so that it picks randomly among ties, rather than picking highest x and y
            }
        }
        return best.second;
    }   
};
struct EarthSTRUCT //contains a more readable map of mars, as well as the code to find where to send a rocket
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
        for (int i = 0; i < r; i++)
        {
            for (int j = 0; j < c; j++)
            {
                bc_MapLocation* loc = new_bc_MapLocation(bc_Planet_earth, i, j);
                earth[i][j] = !bc_PlanetMap_is_passable_terrain_at(map, loc);
               // printf("%d ", earth[i][j]);
            }
          //  printf("\n");
        }
    }
};
MarsSTRUCT mars;
EarthSTRUCT earth
bool check_errors() {
    if (bc_has_err()) {
        char *err;
        /// Note: this clears the current global error.
        int8_t code = bc_get_last_err(&err);
        printf("Engine error code %d: %s\n", code, err);
        bc_free_string(err);
        return true;
    } else {
        return false;
    }
};
/*
/// Earth map.
bc_PlanetMap* bc_GameMap_earth_map_get(bc_GameMap* this);
/// Mars map.
bc_PlanetMap* bc_GameMap_mars_map_get(bc_GameMap* this);
/// The asteroid strike pattern on Mars.
bc_AsteroidPattern* bc_GameMap_asteroids_get(bc_GameMap* this);
/// The orbit pattern that determines a rocket's flight duration.
bc_OrbitPattern* bc_GameMap_orbit_get(bc_GameMap* this);
/// Seed for random number generation.

bc_Planet bc_Player_planet_get(bc_Player* this);
*/
int main() {
    printf("Player C++ bot starting\n");

    srand(0);

    bc_Direction dir = North;
    bc_Direction opposite = bc_Direction_opposite(dir);
    check_errors();

    printf("Opposite direction of %d: %d\n", dir, opposite);

    assert(opposite == South);

    printf("Connecting to manager...\n");

    bc_GameController *gc = new_bc_GameController();

    if (check_errors()) {
        printf("Failed, dying.\n");
        exit(1);
    }
    printf("Connected!\n");
    bc_Planet myPlanet = bc_GameController_planet(gc);
    mars.init(gc);
    while (true) {
        uint32_t round = bc_GameController_round(gc);
        bc_VecUnit *units = bc_GameController_my_units(gc);

        int len = bc_VecUnit_len(units);
        for (int i = 0; i < len; i++) {
            bc_Unit *unit = bc_VecUnit_index(units, i);

            uint16_t id = bc_Unit_id(unit);
            if (bc_GameController_can_move(gc, id, North) && bc_GameController_is_move_ready(gc, id)) {
                bc_GameController_move_robot(gc, id, North);
                check_errors();
            }

            delete_bc_Unit(unit);
        }
        delete_bc_VecUnit(units);

        fflush(stdout);

        bc_GameController_next_turn(gc);
    }
}