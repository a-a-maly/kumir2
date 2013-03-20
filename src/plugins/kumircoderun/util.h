#ifndef UTIL_H
#define UTIL_H

#define DO_NOT_DECLARE_STATIC
#include "vm/variant.hpp"
#include "interfaces/actorinterface.h"
#include <QVariant>

namespace KumirCodeRun {

namespace Util {

using namespace VM;
using namespace Kumir;
using namespace Shared;

QVariant VariableToQVariant(const Variable & var);
AnyValue QVariantToValue(const QVariant & var, int dim);

ActorInterface* findActor(const String & moduleName);

class SleepFunctions: private QThread
{
public:
    inline static void msleep(quint32 msec) {
        QThread::msleep(msec);
    }
    inline static void usleep(quint32 usec) {
        QThread::usleep(usec);
    }
private:
    inline void run() {}
};

}} // namespaces

#endif // UTIL_H
