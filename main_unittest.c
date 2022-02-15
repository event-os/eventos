#include "meow.h"
#include "stdio.h"

// config ----------------------------------------------------------------------
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    meow_unittest_sm();
    //meow_unittest_betree();

    return 0;
}
