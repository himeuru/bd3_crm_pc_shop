#include "Session.h"

Session& Session::instance()
{
    static Session inst;
    return inst;
}
