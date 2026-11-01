# 即时聊天语料进度

目标：100,000 行。语料位于 `resources/dictionary/authoring/im_chat/`，用于语言模型训练，每行是一人发出的一条消息，无角色标记。

从最新 `origin/main` 的 `f17518e` 建立独立分支 `corpus/im-chat`。只新增场景文本及本进度文档，之后在这些文件内续写。全部消息由作者逐条拟写，未复制网站、数据集、书籍或其他输入法词库，未读取 `tests/data/`，没有训练或生成模型。

## 轮次记录

| 轮次 | 日期 | 本轮新增行 | 累计行 | 全局去重行 | 场景文件 |
| --- | --- | ---: | ---: | ---: | ---: |
| 一 | 2026-11-01 | 5,000 | 5,000 | 4,990 | 50 |
| 二 | 2026-11-01 | 5,000 | 10,000 | 9,972 | 50 |
| 三 | 2026-11-01 | 5,000 | 15,000 | 14,922 | 50 |

当前完成目标的 15%。五十个场景各三百行；每个文件内没有完全相同的消息。全局去重后 14,922 行，跨场景自然重复产生的额外行共 78 行，按任务要求保留。

第一轮提交为 `66d4cb9`，覆盖全部基础场景，自查后补入一百九十六条十七至三十字的消息。

第二轮提交为 `959575e`，新增五千行，补充走错入口和时间记岔、交通绕行、漏单和退换寄送、费用核对、借物去向、工作依赖与交接遗漏、预约变更、照看安排。自身去重为 4,993 行，使累计去重行数净增加 4,982 行；原有五千行逐行保留。

第三轮从 `959575e` 续写。开始前重读五十个文件的分散位置，共一百五十条既有消息。各场景物理行数相同，按内容覆盖的不足补充跨天询问进度、通知是否传到、临时安排拆分、异常处理后核对和办完后的收尾。每个场景追加一百条，原有一万行逐行保留。

第三轮自身去重为 4,983 行，使累计去重行数净增加 4,950 行。常见接话和办事回复允许跨文件自然出现，不为提高统计刻意换成罕见说法。

## 长度分布

按汉字数量统计，中文逗号不计入长度。

| 长度 | 第三轮行数 | 第三轮占比 | 累计行数 | 累计占比 |
| --- | ---: | ---: | ---: | ---: |
| 一字 | 0 | 0.00% | 2 | 0.01% |
| 二至三字 | 176 | 3.52% | 370 | 2.47% |
| 四至十六字 | 4677 | 93.54% | 14089 | 93.93% |
| 十七至三十字 | 147 | 2.94% | 539 | 3.59% |
| 超过三十字 | 0 | 0.00% | 0 | 0.00% |

累计一字消息仍为“行”“成”，第三轮没有增加一字消息；所有长消息均不超过三十字。只使用简体汉字和中文逗号，句末没有标点。

## 各场景行数

| 文件 | 第三轮新增 | 累计行数 |
| --- | ---: | ---: |
| `affection_and_banter.txt` | 100 | 300 |
| `apologies_and_repairs.txt` | 100 | 300 |
| `appointments_and_doctors.txt` | 100 | 300 |
| `borrowing_and_returning.txt` | 100 | 300 |
| `child_care.txt` | 100 | 300 |
| `class_and_shift_changes.txt` | 100 | 300 |
| `classes_and_campus.txt` | 100 | 300 |
| `comfort_and_support.txt` | 100 | 300 |
| `computer_and_network.txt` | 100 | 300 |
| `cooking_and_groceries.txt` | 100 | 300 |
| `coworker_handovers.txt` | 100 | 300 |
| `delays_and_rescheduling.txt` | 100 | 300 |
| `elder_care.txt` | 100 | 300 |
| `errands_and_favors.txt` | 100 | 300 |
| `exams_and_revision.txt` | 100 | 300 |
| `exercise_and_gym.txt` | 100 | 300 |
| `family_checkins.txt` | 100 | 300 |
| `file_sharing.txt` | 100 | 300 |
| `food_and_weight_plans.txt` | 100 | 300 |
| `homework_and_projects.txt` | 100 | 300 |
| `leave_requests.txt` | 100 | 300 |
| `leaving_and_home.txt` | 100 | 300 |
| `links_and_information.txt` | 100 | 300 |
| `medicine_and_recovery.txt` | 100 | 300 |
| `meeting_plans.txt` | 100 | 300 |
| `meetings_and_calls.txt` | 100 | 300 |
| `negotiation_and_compromise.txt` | 100 | 300 |
| `parcels_and_shipping.txt` | 100 | 300 |
| `pet_care.txt` | 100 | 300 |
| `phone_problems.txt` | 100 | 300 |
| `photos_and_camera.txt` | 100 | 300 |
| `refusals_and_boundaries.txt` | 100 | 300 |
| `rent_and_utilities.txt` | 100 | 300 |
| `repairs_and_home_visits.txt` | 100 | 300 |
| `restaurant_orders.txt` | 100 | 300 |
| `returns_and_prices.txt` | 100 | 300 |
| `short_replies.txt` | 100 | 300 |
| `sleep_and_bedtime.txt` | 100 | 300 |
| `takeaway_and_meals.txt` | 100 | 300 |
| `taxi_and_transit.txt` | 100 | 300 |
| `thanks_and_reciprocation.txt` | 100 | 300 |
| `traffic_and_routes.txt` | 100 | 300 |
| `transfers_and_splitting.txt` | 100 | 300 |
| `travel_bookings.txt` | 100 | 300 |
| `travel_packing.txt` | 100 | 300 |
| `waiting_and_arrivals.txt` | 100 | 300 |
| `waking_and_alarms.txt` | 100 | 300 |
| `weather_and_clothing.txt` | 100 | 300 |
| `weekend_plans.txt` | 100 | 300 |
| `work_tasks_and_changes.txt` | 100 | 300 |

## 第三轮自查

- 核对新增五千行及累计一万五千行，五十个文件均为 UTF-8 无 BOM、LF 换行；原有一万行与本轮起点逐行相同。
- 检查文件内重复、空行、字符范围、句末标点及长度，全部符合要求；没有英文、阿拉伯数字、表情或角色前缀。
- 用系统简繁转换核对，全部文本转为简体后无差异。
- 写入检查拦截了两条与同文件已有内容完全重复的候选，分别改写后再写入，最终每个场景准确追加一百行。
- 作者抽读新增内容的二百条分散样本，并复核本轮一百四十七条长消息。发现九十九条超过三十字，逐条压短后复查通过。
- 修正生硬搭配和不顺口短句，例如把“你也别只我求你”改为“你有事也记得找我”，没有为避重而保留不自然的语气词。
- 消息直接逐条拟写，没有用地点、菜名或称谓等词槽批量生成；本轮着重补具体办事的跟进和收尾，不根据评测句选写内容。
- 未运行引擎、模型训练或准确率测试，也未读取留出测试集；本轮校验针对语料格式、统计和表达质量。

## 后续补充方向

下一轮开始先重读已有内容，再决定各场景新增分配。优先检查目前是否缺少分歧说清后重新安排、多人协作中私聊确认职责、家人照看时临时换人等具体细节。继续补自然的短接话和省略句，避免把现有对话顺序或人物称谓简单换一遍。
