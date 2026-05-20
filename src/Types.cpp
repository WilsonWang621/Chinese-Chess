#include "Types.h"

Side opponent(Side side) {
    return side == Side::Red ? Side::Black : Side::Red;
}
