#include "Global.h"

namespace SevenWondersDuel {

    ResourceType strToResource(const std::string& s) {
        if(s == "WOOD") return ResourceType::WOOD;
        if(s == "STONE") return ResourceType::STONE;
        if(s == "CLAY") return ResourceType::CLAY;
        if(s == "PAPER") return ResourceType::PAPER;
        if(s == "GLASS") return ResourceType::GLASS;
        return ResourceType::WOOD; // Default
    }

    CardType strToCardType(const std::string& s) {
        if(s == "RAW_MATERIAL") return CardType::RAW_MATERIAL;
        if(s == "MANUFACTURED") return CardType::MANUFACTURED;
        if(s == "MILITARY") return CardType::MILITARY;
        if(s == "SCIENTIFIC") return CardType::SCIENTIFIC;
        if(s == "COMMERCIAL") return CardType::COMMERCIAL;
        if(s == "CIVILIAN") return CardType::CIVILIAN;
        if(s == "GUILD") return CardType::GUILD;
        return CardType::CIVILIAN;
    }

    ScienceSymbol strToScienceSymbol(const std::string& s) {
        if(s == "GLOBE") return ScienceSymbol::GLOBE;
        if(s == "TABLET") return ScienceSymbol::TABLET;
        if(s == "MORTAR") return ScienceSymbol::MORTAR;
        if(s == "COMPASS") return ScienceSymbol::COMPASS;
        if(s == "WHEEL") return ScienceSymbol::WHEEL;
        if(s == "QUILL") return ScienceSymbol::QUILL;
        return ScienceSymbol::NONE;
    }

    std::string resourceToString(ResourceType r) {
        switch(r) {
            case ResourceType::WOOD: return "Wood";
            case ResourceType::STONE: return "Stone";
            case ResourceType::CLAY: return "Clay";
            case ResourceType::PAPER: return "Paper";
            case ResourceType::GLASS: return "Glass";
            default: return "";
        }
    }

}
