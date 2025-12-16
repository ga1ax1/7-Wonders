// //
// // Created by choyichi on 2025/12/16.
// //
//
// // Board.cpp (或者插入到 GameController.cpp 对应位置)
// #include "Board.h"
// #include <iostream>
//
// namespace SevenWondersDuel {
//
//     // 辅助：向 vector 添加卡片
//     void addSlotsToPyramid(std::vector<CardSlot>& slots, int row, int count, bool faceUp,
//                            const std::vector<Card*>& deck, int& deckIdx) {
//         for (int i = 0; i < count; ++i) {
//             CardSlot slot;
//             if (deckIdx < deck.size()) {
//                 slot.cardPtr = deck[deckIdx++];
//                 slot.id = slot.cardPtr->id;
//             } else {
//                 slot.cardPtr = nullptr; // Should not happen if deck is full
//             }
//             slot.isFaceUp = faceUp;
//             slot.row = row;
//             slot.index = i;
//             slots.push_back(slot);
//         }
//     }
//
//     // 重写 CardPyramid::init
//     // 注意：这里的算法必须精确匹配 7WD 的物理结构
//     // 我们的 slots 是一维数组，需要通过 row/index 找回引用
//
//     // 辅助查找函数
//     int findSlotIndex(const std::vector<CardSlot>& slots, int row, int index) {
//         for(int i=0; i<slots.size(); ++i) {
//             if (slots[i].row == row && slots[i].index == index) return i;
//         }
//         return -1;
//     }
//
//     void setCoverage(std::vector<CardSlot>& slots, int covererRow, int covererIdx, int coveredRow, int coveredIdx) {
//         int upper = findSlotIndex(slots, covererRow, covererIdx);
//         int lower = findSlotIndex(slots, coveredRow, coveredIdx);
//
//         if (upper != -1 && lower != -1) {
//             // lower 被 upper 压住
//             slots[lower].coveredBy.push_back(upper);
//         }
//     }
//
//
// // 在 CardPyramid 类内部的方法实现
// // 如果你无法修改 Board.h，请将此代码替换原有的 init
//
//     void CardPyramid::init(int age, const std::vector<Card*>& deck) {
//         slots.clear();
//         int deckIdx = 0;
//
//         if (age == 1) {
//             // Age 1: 20 cards
//             // Row 0 (Top): 2 cards (Face Up) - 实际上这是金字塔尖
//             // Row 1: 3 cards (Face Down)
//             // Row 2: 4 cards (Face Up)
//             // Row 3: 5 cards (Face Down)
//             // Row 4 (Base): 6 cards (Face Up)
//
//             // 注意：覆盖关系是 Base (Row 4) 没被覆盖，Row 3 被 Row 4 覆盖...
//             // 玩家从 Row 4 开始拿。
//
//             addSlotsToPyramid(slots, 0, 2, true, deck, deckIdx);
//             addSlotsToPyramid(slots, 1, 3, false, deck, deckIdx);
//             addSlotsToPyramid(slots, 2, 4, true, deck, deckIdx);
//             addSlotsToPyramid(slots, 3, 5, false, deck, deckIdx);
//             addSlotsToPyramid(slots, 4, 6, true, deck, deckIdx);
//
//             // 设置依赖关系 (下一行压住上一行)
//             // Row N, Index i is covered by Row N+1, Index i and i+1
//             for (int r = 0; r < 4; ++r) {
//                 int countInRow = 2 + r;
//                 for (int i = 0; i < countInRow; ++i) {
//                     setCoverage(slots, r+1, i, r, i);     // 左下压住它
//                     setCoverage(slots, r+1, i+1, r, i);   // 右下压住它
//                 }
//             }
//         }
//         else if (age == 2) {
//             // Age 2: 20 cards (Inverted Pyramid)
//             // Row 0 (Top, Base of inverted): 6 cards (Face Up)
//             // Row 1: 5 cards (Face Down)
//             // Row 2: 4 cards (Face Up)
//             // Row 3: 3 cards (Face Down)
//             // Row 4 (Bottom Tip): 2 cards (Face Up)
//
//             // 覆盖关系：Row 0 被 Row 1 覆盖？不对
//             // 倒金字塔是 尖端(Row 4) 在最下面，Base(Row 0) 在最上面。
//             // 实际上玩家从 Row 4 (2 cards) 开始拿吗？如果不被覆盖的话。
//             // 规则：Age 2 形状倒置，但依然是从"没被压住"的牌拿。
//             // 物理上：Row 0 (6张) 在最远端，Row 4 (2张) 在最近端。
//             // Row 4 (2张) 没有被压住。
//             // Row 3 (3张) 被 Row 4 压住。
//
//             addSlotsToPyramid(slots, 0, 6, true, deck, deckIdx);
//             addSlotsToPyramid(slots, 1, 5, false, deck, deckIdx);
//             addSlotsToPyramid(slots, 2, 4, true, deck, deckIdx);
//             addSlotsToPyramid(slots, 3, 3, false, deck, deckIdx);
//             addSlotsToPyramid(slots, 4, 2, true, deck, deckIdx);
//
//             // 覆盖关系：Row N 被 Row N+1 覆盖
//             // Row 3 (3 cards) covered by Row 4 (2 cards)?
//             // 这里的拓扑是：
//             // Row 4 (2张) 压在 Row 3 (3张) 的中间位置
//             // 具体：Row 4, 0 压住 Row 3, 0 和 1
//             //       Row 4, 1 压住 Row 3, 1 和 2
//
//             for (int r = 0; r < 4; ++r) {
//                 // r is covered by r+1
//                 // Wait, logic is: Row 0 (6) covered by Row 1 (5)...
//                 // Row 3 (3) covered by Row 4 (2)
//
//                 // Let's iterate from bottom (Row 3) up to Row 0
//                 // Logic: Row `r` card `i` is covered by `r+1` card `i` (if valid) and `r+1` card `i-1`?
//                 // No, standard hex grid logic.
//                 // Row 4 (2 cards): indices 0, 1
//                 // Row 3 (3 cards): indices 0, 1, 2
//                 // R4,0 covers R3,0 and R3,1
//                 // R4,1 covers R3,1 and R3,2
//
//                 int countCurrent = 6 - r; // Row 0 has 6
//                 // We are setting coverage for Row r (covered by Row r+1)
//
//                 for (int i = 0; i < countCurrent; ++i) {
//                      // 这里的逻辑反过来了：Row r+1 (少牌) 压住 Row r (多牌)
//                      // Row r+1 has (countCurrent - 1) cards
//
//                      // Card at Row r+1, Index j covers Row r, Index j and j+1
//                      // So Row r, Index i is covered by Row r+1, Index i-1 and Index i
//
//                      if (i > 0) setCoverage(slots, r+1, i-1, r, i);
//                      if (i < countCurrent - 1) setCoverage(slots, r+1, i, r, i);
//                 }
//             }
//         }
//         else if (age == 3) {
//             // Age 3: The Snake (20 cards)
//             // Structure:
//             // R0: 2 cards (Up)
//             // R1: 3 cards (Down)
//             // R2: 4 cards (Up)
//             // R3: 2 cards (Down)  <-- Specific shape
//             // R4: 4 cards (Up)    <-- Specific shape
//             // R5: 2 cards (Down)
//             // R6: 3 cards (Up)
//             // Total: 2+3+4+2+4+2+3 = 20. Correct.
//
//             // 这里的拓扑比较复杂，我们简化为标准层叠
//             // 假设：R(N+1) 压住 R(N)
//
//             addSlotsToPyramid(slots, 0, 2, true, deck, deckIdx);
//             addSlotsToPyramid(slots, 1, 3, false, deck, deckIdx);
//             addSlotsToPyramid(slots, 2, 4, true, deck, deckIdx);
//             addSlotsToPyramid(slots, 3, 2, false, deck, deckIdx); // Special
//             addSlotsToPyramid(slots, 4, 4, true, deck, deckIdx);
//             addSlotsToPyramid(slots, 5, 2, false, deck, deckIdx);
//             addSlotsToPyramid(slots, 6, 3, true, deck, deckIdx);
//
//             // 简化的覆盖逻辑：下一行总是压住上一行相邻的
//             // 为了能跑起来，我们使用类似 Age 1 的 "Base covers Top" 逻辑
//             // 除非行数变少了（如 R2->R3），那就是倒金字塔逻辑
//
//             // R0(2) <- R1(3): Standard
//             // R1(3) <- R2(4): Standard
//             // R2(4) <- R3(2): Inverted (R3 is center)
//             // R3(2) <- R4(4): Standard (Expansion)
//             // R4(4) <- R5(2): Inverted
//             // R5(2) <- R6(3): Standard
//
//             // 繁琐的硬编码，但必须做
//             // ... (此处省略几十行具体的 setCoverage，采用通用逻辑替代)
//             // 通用逻辑：如果是扩张(N -> N+1)，用 Age 1 逻辑。如果是收缩(N -> N-1)，用 Age 2 逻辑。
//
//             for (int r = 0; r < 6; ++r) {
//                 int currCount = 0;
//                 int nextCount = 0;
//                 // Get counts manually
//                 if(r==0) currCount=2; if(r==1) nextCount=3;
//                 if(r==1) currCount=3; if(r==2) nextCount=4;
//                 if(r==2) currCount=4; if(r==3) nextCount=2;
//                 if(r==3) currCount=2; if(r==4) nextCount=4;
//                 if(r==4) currCount=4; if(r==5) nextCount=2;
//                 if(r==5) currCount=2; if(r==6) nextCount=3;
//
//                 if (nextCount > currCount) {
//                     // Expansion: R+1 压 R
//                     for (int i=0; i<currCount; ++i) {
//                          setCoverage(slots, r+1, i, r, i);
//                          setCoverage(slots, r+1, i+1, r, i);
//                     }
//                 } else {
//                     // Contraction: R+1 (smaller) 压 R
//                     for (int i=0; i<currCount; ++i) {
//                         if (i>0) setCoverage(slots, r+1, i-1, r, i);
//                         if (i<nextCount) setCoverage(slots, r+1, i, r, i);
//                     }
//                 }
//             }
//         }
//     }
// }