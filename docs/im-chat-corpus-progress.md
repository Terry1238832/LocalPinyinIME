# 即时聊天语料进度

目标：100,000 行。语料位于 `resources/dictionary/authoring/im_chat/`，用于语言模型训练，每行是一人发出的一条消息，无角色标记。

从最新 `origin/main` 的 `f17518e` 建立独立分支 `corpus/im-chat`。只新增场景文本及本进度文档；全部消息由作者逐条拟写，未复制网站、数据集、书籍或其他输入法词库，未读取 `tests/data/`。此次没有训练或生成模型。

## 轮次记录

| 轮次 | 日期 | 本轮新增行 | 累计行 | 全局去重行 | 场景文件 |
| --- | --- | ---: | ---: | ---: | ---: |
| 一 | 2026-11-01 | 5,000 | 5,000 | 4,990 | 50 |

第一轮覆盖约见与等人、交通、餐饮、快递购物、钱款与借还、学习工作、就医照看、家务报修、设备与文件、旅行、作息及熟人之间的互动。已完成目标的 5%。跨文件允许自然重复，本轮全局重复出现的额外行共 10 行；每个文件内没有完全相同的消息。

## 长度分布

按汉字数量统计，逗号不计入长度。

| 长度 | 行数 | 占比 |
| --- | ---: | ---: |
| 一字 | 1 | 0.02% |
| 二至三字 | 99 | 1.98% |
| 四至十六字 | 4704 | 94.08% |
| 十七至三十字 | 196 | 3.92% |
| 超过三十字 | 0 | 0.00% |

一字消息仅保留“行”，符合任务中列出的简短回复示例。句中只使用简体汉字和中文逗号，句末没有标点。

## 各场景行数

| 文件 | 行数 |
| --- | ---: |
| `affection_and_banter.txt` | 100 |
| `apologies_and_repairs.txt` | 100 |
| `appointments_and_doctors.txt` | 100 |
| `borrowing_and_returning.txt` | 100 |
| `child_care.txt` | 100 |
| `class_and_shift_changes.txt` | 100 |
| `classes_and_campus.txt` | 100 |
| `comfort_and_support.txt` | 100 |
| `computer_and_network.txt` | 100 |
| `cooking_and_groceries.txt` | 100 |
| `coworker_handovers.txt` | 100 |
| `delays_and_rescheduling.txt` | 100 |
| `elder_care.txt` | 100 |
| `errands_and_favors.txt` | 100 |
| `exams_and_revision.txt` | 100 |
| `exercise_and_gym.txt` | 100 |
| `family_checkins.txt` | 100 |
| `file_sharing.txt` | 100 |
| `food_and_weight_plans.txt` | 100 |
| `homework_and_projects.txt` | 100 |
| `leave_requests.txt` | 100 |
| `leaving_and_home.txt` | 100 |
| `links_and_information.txt` | 100 |
| `medicine_and_recovery.txt` | 100 |
| `meeting_plans.txt` | 100 |
| `meetings_and_calls.txt` | 100 |
| `negotiation_and_compromise.txt` | 100 |
| `parcels_and_shipping.txt` | 100 |
| `pet_care.txt` | 100 |
| `phone_problems.txt` | 100 |
| `photos_and_camera.txt` | 100 |
| `refusals_and_boundaries.txt` | 100 |
| `rent_and_utilities.txt` | 100 |
| `repairs_and_home_visits.txt` | 100 |
| `restaurant_orders.txt` | 100 |
| `returns_and_prices.txt` | 100 |
| `short_replies.txt` | 100 |
| `sleep_and_bedtime.txt` | 100 |
| `takeaway_and_meals.txt` | 100 |
| `taxi_and_transit.txt` | 100 |
| `thanks_and_reciprocation.txt` | 100 |
| `traffic_and_routes.txt` | 100 |
| `transfers_and_splitting.txt` | 100 |
| `travel_bookings.txt` | 100 |
| `travel_packing.txt` | 100 |
| `waiting_and_arrivals.txt` | 100 |
| `waking_and_alarms.txt` | 100 |
| `weather_and_clothing.txt` | 100 |
| `weekend_plans.txt` | 100 |
| `work_tasks_and_changes.txt` | 100 |

## 本轮自查

- 五十个文件均为 UTF-8 无 BOM、LF 换行，每个文件一百行，总计五千行。
- 检查了文件内重复、空行、字符范围、句末标点及长度，全部符合要求；没有英文、阿拉伯数字、表情或角色前缀。
- 用系统简繁转换核对，全部文本转为简体后无差异。
- 作者抽读每个文件的三个分散位置，共一百五十条，并逐条复核一百九十六条长消息；这是作者自查，没有声称独立人工审校。
- 初稿长句不足，已将四十九个场景各四条消息改写为十七至三十字的具体交代，保持总数不变。长句均逐条拟写，没有用地点、菜名等词槽批量替换。
- 未运行引擎或准确率测试；本任务的验收针对语料格式、统计及表达质量，不读取留出测试集。

## 下一轮补充方向

本轮场景物理行数均为一百行。下一轮先重读已有消息，重点补充跨步骤交代、多人安排中的私聊确认，以及照看、退款寄送和工作交接中的遗漏情况；把简短接话与长交代分散到各场景。重复的常见回复可保留，但不套用已有十句一组的对话组织方式。
