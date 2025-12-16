//
// Created by choyichi on 2025/12/15.
//

#ifndef SEVEN_WONDERS_DUEL_EFFECTSYSTEM_H
#define SEVEN_WONDERS_DUEL_EFFECTSYSTEM_H

#include "Global.h"
#include <vector>
#include <map>
#include <string>

namespace SevenWondersDuel {

    // 前向声明，避免循环引用
    class Player;
    class GameController;

    // 效果基类
    class IEffect {
    public:
        virtual ~IEffect() = default;

        // 核心：当卡牌/奇迹被建造时触发
        virtual void apply(Player* self, Player* opponent, GameController* ctx) = 0;

        // 核心：游戏结束时计算分数
        virtual int calculateScore(const Player* self, const Player* opponent) const { return 0; }

        // 用于调试/UI显示的描述
        virtual std::string getDescription() const = 0;
    };

    // 1. 资源产出 (木/石/土/纸/玻)
    class ProductionEffect : public IEffect {
    public:
        std::map<ResourceType, int> producedResources;
        bool isChoice; // true=多选一 (如黄卡/奇迹), false=全部产出 (如灰卡/棕卡)

        ProductionEffect(std::map<ResourceType, int> res, bool choice = false)
            : producedResources(res), isChoice(choice) {}

        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override;
    };

    // 2. 军事效果 (红卡/奇迹)
    class MilitaryEffect : public IEffect {
    public:
        int shields;

        explicit MilitaryEffect(int count) : shields(count) {}
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override;
    };

    // 3. 科技效果 (绿卡/法律)
    class ScienceEffect : public IEffect {
    public:
        ScienceSymbol symbol;

        explicit ScienceEffect(ScienceSymbol s) : symbol(s) {}
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override;
    };

    // 4. 直接给分 (蓝卡/绿卡/奇迹)
    class VictoryPointEffect : public IEffect {
    public:
        int points;

        explicit VictoryPointEffect(int p) : points(p) {}
        void apply(Player* self, Player* opponent, GameController* ctx) override; // 通常为空
        int calculateScore(const Player* self, const Player* opponent) const override;
        std::string getDescription() const override;
    };

    // 5. 金币效果 (立即获得)
    class CoinEffect : public IEffect {
    public:
        int amount;

        explicit CoinEffect(int a) : amount(a) {}
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override;
    };

    // 6. 条件金币效果 (如：每有一个黄卡给1金币) - 对应黄卡/奇迹
    class CoinsPerTypeEffect : public IEffect {
    public:
        CardType targetType; // 统计哪种卡
        int coinsPerCard;
        bool countWonder;    // 是否统计奇迹 (竞技场)

        CoinsPerTypeEffect(CardType type, int amount, bool wonder = false)
            : targetType(type), coinsPerCard(amount), countWonder(wonder) {}

        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override;
    };

    // 7. 交易优惠 (黄卡：石/木/土/纸/玻 价格变为1)
    class TradeDiscountEffect : public IEffect {
    public:
        ResourceType resource;

        explicit TradeDiscountEffect(ResourceType r) : resource(r) {}
        void apply(Player* self, Player* opponent, GameController* ctx) override; // 设置玩家标志位
        std::string getDescription() const override;
    };

    // 8. 摧毁卡牌 (宙斯/竞技场)
    class DestroyCardEffect : public IEffect {
    public:
        CardType targetColor; // 只能摧毁特定颜色的卡

        explicit DestroyCardEffect(CardType color) : targetColor(color) {}
        void apply(Player* self, Player* opponent, GameController* ctx) override; // 触发状态切换
        std::string getDescription() const override;
    };

    // 9. 额外回合 (奇迹)
    class ExtraTurnEffect : public IEffect {
    public:
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override { return "Take another turn immediately."; }
    };

    // 10. 从弃牌堆建造 (陵墓)
    class BuildFromDiscardEffect : public IEffect {
    public:
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override { return "Build a card from discard pile for free."; }
    };

    // 11. 发展标记选择 (图书馆)
    class ProgressTokenSelectEffect : public IEffect {
    public:
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override { return "Choose a progress token from the box."; }
    };

    // 12. 强迫对手丢钱 (亚壁古道)
    class OpponentLoseCoinsEffect : public IEffect {
    public:
        int amount;
        explicit OpponentLoseCoinsEffect(int a) : amount(a) {}
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        std::string getDescription() const override;
    };

    // 13. 行会效果 (紫色卡)
    // 逻辑较复杂，通常包含两部分：立即给钱，结束给分
    enum class GuildCriteria {
        YELLOW_CARDS,      // 双方最多的黄卡
        BROWN_GREY_CARDS,  // 双方最多的棕+灰
        WONDERS,           // 双方最多的奇迹
        BLUE_CARDS,        // 双方最多的蓝卡
        GREEN_CARDS,       // 双方最多的绿卡
        RED_CARDS,         // 双方最多的红卡
        COINS              // 双方最多的金币 (/3)
    };

    class GuildEffect : public IEffect {
    public:
        GuildCriteria criteria;

        explicit GuildEffect(GuildCriteria c) : criteria(c) {}

        // 立即效果：给钱
        void apply(Player* self, Player* opponent, GameController* ctx) override;
        // 结束效果：给分
        int calculateScore(const Player* self, const Player* opponent) const override;

        std::string getDescription() const override;
    };

}

#endif // SEVEN_WONDERS_DUEL_EFFECTSYSTEM_H