#include <ultra64.h>

#include <types.h>

#include "main.h"
#include "applo.h"
#include "world.h"

void applo_main(unused void *arg)
{
    while (true)
    {
        world_update_chunk(&app_world);
    }
}
