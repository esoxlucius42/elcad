#include <iostream>

// Entry point wrapper to call the implementation compiled into elcad target's translation unit
extern int sphere_tess_count_main(int argc, char** argv);

int main(int argc, char** argv) {
    return sphere_tess_count_main(argc, argv);
}
