#include "WarehouseController.h"
#include "../core/Session.h"

bool WarehouseController::replenish(int fillTo)
{
    WarehouseRepository repo;
    auto& s = Session::instance();
    return repo.replenish(s.shopId, s.userId, fillTo);
}
