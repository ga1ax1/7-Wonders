#include "RenderContext.h"

namespace SevenWondersDuel {

    void RenderContext::clear() {
        cardIdMap.clear();
        wonderIdMap.clear();
        tokenIdMap.clear();
        oppCardIdMap.clear();
        discardIdMap.clear();
        boxTokenIdMap.clear();
        draftWonderIds.clear();
    }

}
