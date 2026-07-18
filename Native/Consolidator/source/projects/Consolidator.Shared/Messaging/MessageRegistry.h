#pragma once

#include "MessageFactory.h"
#include "Messages/AnalyzerDifferenceMessage.h"
#include "Messages/ApproximatorClearMessage.h"
#include "Messages/ApproximatorFitMessage.h"
#include "Messages/EqBankChangedMessage.h"
#include "Messages/EqSnapshotMessage.h"
#include "Messages/FeatureStatusMessage.h"
#include "Messages/FilterApplyMessage.h"
#include "Messages/FilterChangedMessage.h"
#include "Messages/FilterDefinitionMessage.h"
#include "Messages/FilterEditMessage.h"
#include "Messages/FilterRestoreMessage.h"
#include "Messages/SystemStartMessage.h"

namespace consolidator::messaging {

class MessageRegistry final {
public:
    static MessageFactory CreateFactory() {
        MessageFactory factory;
        Register(factory);
        return factory;
    }

    static void Register(MessageFactory& factory) {
        factory.Register<AnalyzerDifferenceMessage>();
        factory.Register<ApproximatorClearMessage>();
        factory.Register<ApproximatorFitMessage>();
        factory.Register<EqBankChangedMessage>();
        factory.Register<EqSnapshotMessage>();
        factory.Register<FeatureStatusMessage>();
        factory.Register<FilterApplyMessage>();
        factory.Register<FilterChangedMessage>();
        factory.Register<FilterDefinitionMessage>();
        factory.Register<FilterEditMessage>();
        factory.Register<FilterRestoreMessage>();
        factory.Register<SystemStartMessage>();
    }
};

} // namespace consolidator::messaging
