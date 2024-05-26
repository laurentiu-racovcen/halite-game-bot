#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <map>
#include <fstream>
#include <cmath>

#include "hlt.hpp"
#include "networking.hpp"

#define UNOCCUPIED_ID 0

using namespace std;

/* Functie care returneaza directie de la "from_location" spre "to_location" */
unsigned char get_direction(hlt::Location from_location, hlt::Location to_location, hlt::GameMap presentMap) {
    float angle = presentMap.getAngle(from_location, to_location);

    // daca unghiul obtinut e negativ, atunci i se adauga 2*pi
    if (angle < 0) {
        angle += 2*M_PI;
    }

    if (angle > M_PI_4 && angle <= 3*M_PI_4) {
        return SOUTH;
    } else if (angle > 3*M_PI_4 && angle <= 5*M_PI_4) {
        return WEST;
    } else if (angle > 5*M_PI_4 && angle <= 7*M_PI_4) {
        return NORTH;
    } else {
        return EAST;
    }
}

/* Functie care returnează direcția spre care obținem o putere de atac mai mare */
unsigned int get_max_overkill_direction(hlt::Location from_location, vector<hlt::Location> to_locations, hlt::GameMap presentMap) {
    unsigned int overkill = 0;
    hlt::Location next_location;
    for (size_t i = 0; i < to_locations.size(); i++) {
        unsigned int current_overkill = 0;
        for (size_t j = 1; j <= 4; j++) {
            hlt::Location neigh_location = presentMap.getLocation(to_locations[i], j);
            // verificam ca ownerii sa fie enemies
            if (presentMap.getSite(neigh_location).owner != UNOCCUPIED_ID
                && presentMap.getSite(neigh_location).owner != presentMap.getSite(from_location).owner) {
                    current_overkill += presentMap.getSite(neigh_location).strength;
                }
        }
        if (current_overkill > overkill) {
            overkill = current_overkill;
            next_location = to_locations[i];
        }
    }

    return get_direction(from_location, next_location, presentMap);
}

/* Functie care returneaza urmatoarea miscare a unei piese */
unsigned char get_next_direction(hlt::GameMap presentMap, hlt::Location location, FILE* fout) {
    hlt::Site piece = presentMap.getSite(location);

    if (piece.strength == 0) {
        return STILL;
    }

    vector<hlt::Location> unoccupied_pieces;
    vector<hlt::Location> enemy_pieces;

    for (unsigned char i = 1; i <= 4; i++) {
        hlt::Location neigh_location = presentMap.getLocation(location, i);

        if (presentMap.getSite(neigh_location).owner == UNOCCUPIED_ID) {
            unoccupied_pieces.push_back(neigh_location);
        } else if (presentMap.getSite(neigh_location).owner != piece.owner)
        {
            enemy_pieces.push_back(neigh_location);
        }
    }

    // daca piesa are minim un vecin enemy
    // se returneaza directia vecinului cu max overkill
    if (enemy_pieces.size() != 0) {
        return get_max_overkill_direction(location, enemy_pieces, presentMap);
    }

    // daca piesa are minim un vecin neocupat
    // se alege vecinul conform raportului maxim de prod / strength
    else if (unoccupied_pieces.size() != 0) {
        
        unsigned char next_direction = 0;
        unsigned char strength;
        double max_ratio = -DBL_MAX;

        for (size_t i = 0; i < unoccupied_pieces.size(); i++) {
            hlt::Site current_site = presentMap.getSite(unoccupied_pieces[i]);
            if (current_site.strength != 0) {
                if (1.0*current_site.production / current_site.strength > max_ratio) {
                    max_ratio = 1.0*current_site.production / current_site.strength;
                    next_direction = get_direction(location, unoccupied_pieces[i], presentMap);
                    strength = current_site.strength;
                }
            } else {
                 // returneaza directia vecinului cu strength = 0
                return get_direction(location, unoccupied_pieces[i], presentMap);
            }
        }

        if (strength > piece.strength) {
            return STILL;
        }

        return next_direction;
    } else {
        /* toti vecinii apartin bot-ului */

        if (piece.strength <= 4 * piece.production) {
            return STILL;
        }

        unsigned short min_dist = numeric_limits<unsigned short>::max();
        hlt::Location best_location = {0,0};

        // se verifica daca harta contine enemies
        bool map_contains_enemies = false;
        for (unsigned short i = 0; i < presentMap.height; i++) {
            for (unsigned short j = 0; j < presentMap.width; j++) {
                if (presentMap.getSite({j,i}).owner != piece.owner && presentMap.getSite({j,i}).owner != 0) {
                    map_contains_enemies = true;
                    break;
                }
            }
        }

        if (map_contains_enemies) {
            /* harta contine enemies */

            // se cauta min dist pana la un enemy
            for (unsigned short i = 0; i < presentMap.height; i++) {
                for (unsigned short j = 0; j < presentMap.width; j++) {                    
                    if (presentMap.getSite({j,i}).owner != piece.owner && presentMap.getSite({j,i}).owner != 0) {
                        unsigned short current_dist = presentMap.getDistance(location, {j, i});

                        // compara distanta
                        if (min_dist >= current_dist) {
                            best_location = {j, i};
                            min_dist = current_dist;
                        }
                    }
                }
            }
        } else {
            /* harta nu contine enemies */
            double max_ratio = -1;
            for (unsigned short i = 0; i < presentMap.height; i++) {
                for (unsigned short j = 0; j < presentMap.width; j++) {
                    // asociem fiecarei piese neocupate un raport
                    // dintre production si distanta pana la aceasta
                    if (presentMap.getSite({j,i}).owner != piece.owner) {
                        unsigned short current_dist = presentMap.getDistance(location, {j, i});
                        unsigned char current_prod = presentMap.getSite({j, i}).production;

                        if ((1.0 * current_prod)/current_dist > max_ratio) {
                            max_ratio = (1.0 * current_prod)/current_dist;
                            fprintf(fout, "ratio = %f\n", max_ratio);
                            best_location = {j, i};
                        }
                    }
                }
            }
        }

        return get_direction(location, best_location, presentMap);
    }

    return -1;
}

/* Functie care returneaza opusul directiei primite ca parametru */
unsigned char opposite_dir(unsigned char dir) {
    if (dir == NORTH) {
        return SOUTH;
    } else if (dir == SOUTH) {
        return NORTH;
    } else if (dir == EAST) {
        return WEST;
    } else if (dir == WEST) {
        return EAST;
    }
    return STILL;
}

/* Functie care verifica daca harta contine enemies */
bool map_contains_enemies( unsigned char myID, hlt::GameMap presentMap) {
    for (unsigned short i = 0; i < presentMap.height; i++) {
        for (unsigned short j = 0; j < presentMap.width; j++) {
            if (presentMap.getSite({j,i}).owner != myID && presentMap.getSite({j,i}).owner != 0) {
                return true;
                break;
            }
        }
    }
    return false;
}

/* Functie care actualizeaza directia pieselor in functie de piesele adiacente
(pentru a nu avea pierderi de strength) */
std::set<hlt::Move> update_directions(std::set<hlt::Move> moves, hlt::GameMap presentMap, unsigned char myID, FILE* fout) {

    std::map<hlt::Location, unsigned char> moves_map;
    std::set<hlt::Move> new_moves;

    // utilizand "moves_set", se mapeaza locatiile la directiile acestora in "moves_map"
    for (set<hlt::Move>::iterator iter = moves.begin(); iter != moves.end(); iter++) {
        moves_map.insert({iter->loc, iter->dir});
    }

    for (std::map<hlt::Location, unsigned char>::iterator iter = moves_map.begin(); iter != moves_map.end(); iter++) {

        if (iter->second == STILL) {
            continue;
        }

        hlt::Site iter_site = presentMap.getSite(iter->first);
        hlt::Location iter_next_location = presentMap.getLocation(iter->first, iter->second);
        if (iter_site.owner == myID) {

            // pentru fiecare vecin direct, se verifica vecinii acestuia
            for (unsigned char i = 1; i <= 4; i++) {
                hlt::Location neigh_location = presentMap.getLocation(iter->first, i);
                hlt::Location neigh_target_location = presentMap.getLocation(neigh_location, moves_map[neigh_location]);
                unsigned char neigh_strength = presentMap.getSite(neigh_location).strength;
                bool is_checked = false;
                if ((presentMap.getSite(neigh_location).owner == myID)) {

                    // se verifica mai intai vecinul direct
                    if ((iter_site.strength + neigh_strength > 255)
                        && (iter_next_location.x == neigh_target_location.x)
                        && (iter_next_location.y == neigh_target_location.y)
                        && (presentMap.getSite(neigh_location).owner == myID)) {
                        is_checked = true;
                        // se actualizeaza directia locatiei in "moves_map"
                        moves_map[iter->first] = STILL;
                        iter = moves_map.begin();
                        break;
                    }

                    // verifica apoi vecinii vecinului direct curent
                    if (!is_checked) {
                        for (unsigned char j = 1; j <= 4; j++) {
                            // daca j != opusul lui i
                            if (j != opposite_dir(i)) {
                                hlt::Location neigh_neigh_location = presentMap.getLocation(neigh_location, j);
                                unsigned char neigh_neigh_strength = presentMap.getSite(neigh_neigh_location).strength;
                                hlt::Location neigh_neigh_target_location =
                                    presentMap.getLocation(neigh_neigh_location, moves_map[neigh_neigh_location]);

                                if ((iter_site.strength + neigh_neigh_strength > 255)
                                        && (iter_next_location.x == neigh_neigh_target_location.x)
                                        && (iter_next_location.y == neigh_neigh_target_location.y)
                                        && (presentMap.getSite(neigh_neigh_location).owner == myID
                                            || (presentMap.getSite(neigh_location).owner != myID
                                            && presentMap.getSite(neigh_location).owner != 0))) {
                                    is_checked = true;
                                    // se actualizeaza directia locatiei in "moves_map"
                                    moves_map[iter->first] = STILL;
                                    iter = moves_map.begin();
                                    break;
                                }
                            }
                        }
                    }
                }
                if (is_checked) {
                    break;
                }
            }
        }
    }

    // insereaza elementele din "moves_map" in "new_moves"
    for (std::map<hlt::Location, unsigned char>::iterator iter = moves_map.begin(); iter != moves_map.end(); iter++) {
        new_moves.insert({ iter->first, iter->second });
    }

    return new_moves;
}

int main() {

    FILE* fout;
    fout = fopen ("test", "a");

    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    sendInit("VALgrind");

    std::set<hlt::Move> moves;

    unsigned char current_move = 0;

    int frames_number = 0;

    while(true) {
        moves.clear();

        getFrame(presentMap);

        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {

                hlt::Location location;

                location.x = b;
                location.y = a;

                if (presentMap.getSite({ b, a }).owner == myID) {
                    current_move = get_next_direction(presentMap, location, fout);
                    moves.insert({ { b, a }, current_move });
                }
            }
        }
        frames_number++;

        if (!map_contains_enemies(myID, presentMap)) {
            sendFrame(moves);
        } else {
            // optimizarea directiilor pentru maximizarea strength-ului total
            // al bot-ului
            sendFrame(update_directions(moves, presentMap, myID, fout));
        }
    }

    return 0;
}
