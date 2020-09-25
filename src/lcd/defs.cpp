#include "defs.hpp"


namespace gbaemu::lcd
{
    const char *layerIDToString(LayerID id)
    {
        switch (id) {
            case LAYER_BG0: return "LAYER_BG0";
            case LAYER_BG1: return "LAYER_BG1";
            case LAYER_BG2: return "LAYER_BG2";
            case LAYER_BG3: return "LAYER_BG3";
            case LAYER_OBJ0: return "LAYER_OBJ0";
            case LAYER_OBJ1: return "LAYER_OBJ1";
            case LAYER_OBJ2: return "LAYER_OBJ2";
            case LAYER_OBJ3: return "LAYER_OBJ3";
            default: return "INVALID";
        }
    }
}