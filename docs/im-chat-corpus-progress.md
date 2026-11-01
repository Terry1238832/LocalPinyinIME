# 即时聊天语料进度

目标：100,000 行。语料位于 `resources/dictionary/authoring/im_chat/`，用于语言模型训练，每行是一人发出的一条消息，无角色标记。

从最新 `origin/main` 的 `f17518e` 建立独立分支 `corpus/im-chat`。只新增场景文本及本进度文档，之后在这些文件内续写。全部消息由作者逐条拟写，未复制网站、数据集、书籍或其他输入法词库，未读取 `tests/data/`，没有训练或生成模型。

## 轮次记录

| 轮次 | 日期 | 本轮新增行 | 累计行 | 全局去重行 | 场景文件 |
| --- | --- | ---: | ---: | ---: | ---: |
| 一 | 2026-11-01 | 5,000 | 5,000 | 4,990 | 50 |
| 二 | 2026-11-01 | 5,000 | 10,000 | 9,972 | 50 |

当前完成目标的 10%。五十个场景各二百行；每个文件内没有完全相同的消息。全局去重后 9,972 行，跨场景自然重复产生的额外行共 28 行，按任务要求保留。

第一轮提交为 `66d4cb9`，覆盖全部基础场景，并在自查后补入一百九十六条十七至三十字的消息。

第二轮从 `66d4cb9` 续写，重读了五十个文件的分散位置，共二百五十条既有消息。场景物理行数相同，因此按内容覆盖的不足补充：走错入口和时间记岔、交通临时绕行、漏单和退换寄送、费用核对、借物去向、任务依赖与交接遗漏、预约变更、私聊确认与照看安排。每个场景追加一百条，原有五千条逐行保留。

第二轮自身去重为 4,993 行，使累计全局去重行数净增加 4,982 行。重复主要是自然的接话和办事回复，不为提高统计而刻意改写成罕见说法。

## 长度分布

按汉字数量统计，中文逗号不计入长度。

| 长度 | 第二轮行数 | 第二轮占比 | 累计行数 | 累计占比 |
| --- | ---: | ---: | ---: | ---: |
| 一字 | 1 | 0.02% | 2 | 0.02% |
| 二至三字 | 95 | 1.90% | 194 | 1.94% |
| 四至十六字 | 4708 | 94.16% | 9412 | 94.12% |
| 十七至三十字 | 196 | 3.92% | 392 | 3.92% |
| 超过三十字 | 0 | 0.00% | 0 | 0.00% |

累计一字消息为“行”“成”，其余短回复以二至三字为主；所有长消息均不超过三十字。句中只使用简体汉字和中文逗号，句末没有标点。

## 各场景行数

| 文件 | 第二轮新增 | 累计行数 |
| --- | ---: | ---: |
| `affection_and_banter.txt` | 100 | 200 |
| `apologies_and_repairs.txt` | 100 | 200 |
| `appointments_and_doctors.txt` | 100 | 200 |
| `borrowing_and_returning.txt` | 100 | 200 |
| `child_care.txt` | 100 | 200 |
| `class_and_shift_changes.txt` | 100 | 200 |
| `classes_and_campus.txt` | 100 | 200 |
| `comfort_and_support.txt` | 100 | 200 |
| `computer_and_network.txt` | 100 | 200 |
| `cooking_and_groceries.txt` | 100 | 200 |
| `coworker_handovers.txt` | 100 | 200 |
| `delays_and_rescheduling.txt` | 100 | 200 |
| `elder_care.txt` | 100 | 200 |
| `errands_and_favors.txt` | 100 | 200 |
| `exams_and_revision.txt` | 100 | 200 |
| `exercise_and_gym.txt` | 100 | 200 |
| `family_checkins.txt` | 100 | 200 |
| `file_sharing.txt` | 100 | 200 |
| `food_and_weight_plans.txt` | 100 | 200 |
| `homework_and_projects.txt` | 100 | 200 |
| `leave_requests.txt` | 100 | 200 |
| `leaving_and_home.txt` | 100 | 200 |
| `links_and_information.txt` | 100 | 200 |
| `medicine_and_recovery.txt` | 100 | 200 |
| `meeting_plans.txt` | 100 | 200 |
| `meetings_and_calls.txt` | 100 | 200 |
| `negotiation_and_compromise.txt` | 100 | 200 |
| `parcels_and_shipping.txt` | 100 | 200 |
| `pet_care.txt` | 100 | 200 |
| `phone_problems.txt` | 100 | 200 |
| `photos_and_camera.txt` | 100 | 200 |
| `refusals_and_boundaries.txt` | 100 | 200 |
| `rent_and_utilities.txt` | 100 | 200 |
| `repairs_and_home_visits.txt` | 100 | 200 |
| `restaurant_orders.txt` | 100 | 200 |
| `returns_and_prices.txt` | 100 | 200 |
| `short_replies.txt` | 100 | 200 |
| `sleep_and_bedtime.txt` | 100 | 200 |
| `takeaway_and_meals.txt` | 100 | 200 |
| `taxi_and_transit.txt` | 100 | 200 |
| `thanks_and_reciprocation.txt` | 100 | 200 |
| `traffic_and_routes.txt` | 100 | 200 |
| `transfers_and_splitting.txt` | 100 | 200 |
| `travel_bookings.txt` | 100 | 200 |
| `travel_packing.txt` | 100 | 200 |
| `waiting_and_arrivals.txt` | 100 | 200 |
| `waking_and_alarms.txt` | 100 | 200 |
| `weather_and_clothing.txt` | 100 | 200 |
| `weekend_plans.txt` | 100 | 200 |
| `work_tasks_and_changes.txt` | 100 | 200 |

## 第二轮自查

- 核对新增五千行及累计一万行，五十个文件均为 UTF-8 无 BOM、LF 换行；原有五千行与本轮起点逐行相同。
- 检查文件内重复、空行、字符范围、句末标点及长度，全部符合要求；没有英文、阿拉伯数字、表情或角色前缀。
- 用系统简繁转换核对，全部文本转为简体后无差异。
- 作者抽读新增内容的二百条分散样本，并复核本轮一百九十六条长消息，检查具体交代、语气和上下文是否自然。
- 发现九条超过三十字的消息，逐条压短后复查通过；另把“我去承担改期的事”等两处偏书面表达改得更口语化。
- 消息直接逐条拟写，没有用地点、菜名、人物称谓等词槽批量生成。相似场景使用不同的具体问题、动作和回复，没有根据评测失败句选写内容。
- 未运行引擎、模型训练或准确率测试，也未读取留出测试集；本轮校验针对语料格式、统计和表达质量。

## 下一轮补充方向

各场景物理行数均衡，继续以实际办事环节的覆盖不足为依据。下一轮开始先重读已有内容，优先考虑跨天跟进、第三人转达后的私聊确认、异常恢复与结束确认；增加自然省略和分散在各场景的短接话，避免反复使用现有对话推进顺序。
