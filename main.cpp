#include "bits/stdc++.h"

// oh god
#define this it
#include <bc.h>
#undef this

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
}

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

    while (true) {
        uint32_t round = bc_GameController_round(gc);
        printf("Round: %d\n", round);

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