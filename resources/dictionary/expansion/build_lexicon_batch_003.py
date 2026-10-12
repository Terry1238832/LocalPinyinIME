#!/usr/bin/env python3
"""Author batch003: independently written practical short Chinese vocabulary.

The pypinyin package is an authoring-only pronunciation aid. No external word
list, corpus, article, benchmark answer or user input is imported. This script
writes only expansion artifacts and never changes the runtime dictionaries.
"""
from __future__ import annotations

import hashlib
import json
import re
from collections import Counter
from pathlib import Path

from pypinyin import Style, lazy_pinyin

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent

# Each item is an individually chosen reusable word, name of an ordinary dish,
# practical object, or conventional expression. No Cartesian generation.
SECTIONS = [
    ("food_ingredients_and_flavours", 5500, """
    食材 配菜 荤菜 素菜 主菜 小菜 蘸料 蘸水 酱料 底料 馅料 馅心
    调味料 香辛料 复合调料 火锅底料 麻辣底料 鲜味 酸味 甜味 辣味
    咸味 苦味 涩味 腥味 膻味 糊味 焦香 奶香 麦香 果香 酒香
    烟熏味 香辣 酸辣 甜辣 鲜辣 蒜香 葱香 椒香 咸香 清甜 甘甜
    回甘 爽口 开胃 解腻 微辣 中辣 特辣 不辣 少盐 少糖 低盐
    低糖 无糖 无盐 无油 少油 无添加 代糖 零糖 零脂 低脂
    脱脂 全脂 半脱脂 高蛋白 膳食纤维 热量 营养素 饱腹感 升糖
    面粉 高筋粉 低筋粉 中筋粉 全麦粉 玉米粉 糯米粉 粘米粉
    米粉 红薯粉 木薯粉 土豆粉 豌豆粉 绿豆粉 淀粉 生粉 藕粉
    面包糠 面包屑 泡打粉 小苏打 酵母粉 酵母 甜酒曲 酒曲
    食用碱 植物油 菜籽油 花生油 玉米油 葵花油 橄榄油 山茶油
    芝麻油 香油 辣椒油 花椒油 藤椒油 红油 葱油 猪油 黄油
    淡奶油 炼乳 奶酪 芝士 马苏里拉 奶油奶酪 奶粉 可可粉
    抹茶粉 咖啡粉 椰浆 椰奶 燕麦奶 豆乳 杏仁奶 黑芝麻粉
    番茄酱 沙拉酱 甜面酱 黄豆酱 豆瓣酱 剁椒酱 辣椒酱 蒜蓉酱
    芝麻酱 花生酱 黄芥末 黑胡椒 白胡椒 红椒粉 孜然粉 咖喱粉
    五香粉 十三香 花椒 藤椒 八角 桂皮 香叶 草果 丁香 肉桂
    小茴香 迷迭香 百里香 罗勒 欧芹 香菜 芫荽 薄荷 紫苏
    香茅 香草 香草精 料酒 黄酒 米酒 白醋 米醋 陈醋 香醋
    苹果醋 蚝油 生抽 老抽 蒸鱼豉油 鱼露 味醂 味噌 腐乳
    红腐乳 南乳 豆豉 冰糖 白砂糖 红糖 黄糖 黑糖 糖粉 蜂蜜
    糖浆 枫糖 浓缩汁 柠檬汁 柠檬皮 橙皮 陈皮 枸杞 红枣
    桂圆 核桃仁 杏仁片 巴旦木 腰果 碧根果 开心果 夏威夷果
    松子仁 南瓜籽 葵花籽 白芝麻 黑芝麻 亚麻籽 奇亚籽 莲子
    芡实 薏米 红豆 绿豆 芸豆 鹰嘴豆 豌豆 黄豆 黑豆 蚕豆
    毛豆 豆角 荷兰豆 四季豆 扁豆 豇豆 刀豆 秋葵 苦瓜 丝瓜
    冬瓜 南瓜 西葫芦 黄瓜 笋瓜 莴笋 莴苣 生菜 油麦菜 菠菜
    小白菜 上海青 菜心 娃娃菜 包菜 卷心菜 紫甘蓝 芥蓝 芥菜
    空心菜 木耳菜 苋菜 茼蒿 贡菜 芹菜 西芹 韭菜 韭黄 蒜苗
    蒜薹 小葱 大葱 洋葱 红葱头 生姜 嫩姜 老姜 蒜头 蒜瓣
    西蓝花 西兰花 菜花 花菜 花椰菜 胡萝卜 白萝卜 水萝卜 青萝卜
    土豆 山药 芋头 魔芋 莲藕 竹笋 春笋 冬笋 茭白 荸荠 马蹄
    慈姑 百合 芦笋 茄子 长茄 圆茄 番茄 圣女果 青椒 红椒 黄椒
    彩椒 小米椒 杭椒 尖椒 螺丝椒 线椒 干辣椒 朝天椒 二荆条
    口蘑 香菇 杏鲍菇 白玉菇 蟹味菇 金针菇 海鲜菇 平菇 鸡腿菇
    竹荪 羊肚菌 松茸 茶树菇 猴头菇 黑木耳 银耳 干木耳 干香菇
    鲜香菇 黄花菜 海带 紫菜 裙带菜 海苔 豆芽 绿豆芽 黄豆芽
    豆腐 嫩豆腐 老豆腐 内酯豆腐 水豆腐 油豆腐 冻豆腐 豆腐皮
    干豆腐 豆干 香干 千张 腐竹 素鸡 素肉 面筋 油面筋 烤麸
    鸡胸肉 鸡腿肉 鸡翅根 鸡翅中 鸡里脊 鸡爪 鸡胗 鸡肝 鸡心
    鸭腿 鸭翅 鸭掌 鸭胗 鸭血 鸡蛋 鸭蛋 鹌鹑蛋 咸鸭蛋 松花蛋
    皮蛋 肉末 肉馅 肉糜 肉片 肉丝 肉丁 五花肉 前腿肉 后腿肉
    里脊肉 梅花肉 排骨 肋排 仔排 棒骨 扇骨 猪蹄 猪肚 猪耳
    猪心 猪肝 猪血 肥肠 小肠 腊肉 腊肠 火腿 培根 肉松 牛腱
    牛腩 牛肋条 牛里脊 牛百叶 毛肚 黄喉 牛筋 牛尾 羊排 羊腿
    羊蝎子 羊肉卷 肥牛卷 蟹棒 鱼丸 虾丸 墨鱼丸 贡丸 牛肉丸
    鳕鱼 三文鱼 鲈鱼 鲫鱼 草鱼 带鱼 黄花鱼 鲅鱼 鲳鱼 秋刀鱼
    多宝鱼 龙利鱼 巴沙鱼 银鳕鱼 金枪鱼 沙丁鱼 青花鱼 罗非鱼
    鱿鱼 章鱼 墨鱼 海虾 基围虾 白虾 虾仁 虾皮 虾米 海米
    螃蟹 梭子蟹 大闸蟹 青蟹 蟹黄 蟹膏 蛤蜊 花甲 蛏子 扇贝
    生蚝 牡蛎 青口 贝柱 干贝 鲍鱼 海参 海蜇 鱼籽 鱼皮 鱼胶
    河粉 米线 粉丝 粉条 宽粉 苕粉 米皮 肠粉 拉面 挂面 刀削面
    乌冬面 荞麦面 意面 通心粉 螺旋面 贝壳面 年糕 饺子皮 馄饨皮
    春卷皮 手抓饼 飞饼 饼皮 面皮 吐司 全麦吐司 贝果 可颂 法棍
    欧包 碱水包 餐包 汉堡胚 面包片 米饭 杂粮饭 糙米饭 紫米饭
    燕麦 藜麦 大麦 小米 高粱 黑米 紫米 糙米 胚芽米 香米
    """),
    ("ordinary_dishes_and_drinks", 5500, """
    番茄炒蛋 西红柿汤 土豆丝 凉拌黄瓜 拍黄瓜 蒜蓉生菜 蚝油生菜
    清炒时蔬 炒青菜 炒豆芽 炒三丝 炒双菇 干煸豆角 地三鲜
    鱼香茄子 肉末茄子 蒜蓉茄子 红烧茄子 虎皮青椒 香菇青菜
    手撕包菜 酸辣白菜 白菜豆腐 炒合菜 西芹百合 韭菜炒蛋
    木须肉 京酱肉丝 鱼香肉丝 青椒肉丝 青椒肉片 宫保鸡丁
    辣子鸡 黄焖鸡 小炒肉 回锅肉 红烧肉 糖醋里脊 糖醋排骨
    红烧排骨 清蒸排骨 蒜香排骨 椒盐排骨 粉蒸肉 粉蒸排骨
    梅菜扣肉 蒸肉饼 蒸蛋羹 水蒸蛋 茶叶蛋 卤鸡蛋 煎鸡蛋
    水煮蛋 荷包蛋 溏心蛋 炒鸡蛋 蛋包饭 蛋炒饭 扬州炒饭
    咖喱饭 卤肉饭 煲仔饭 盖浇饭 木桶饭 便当饭 团子 饭团
    紫菜包饭 寿司 手卷 泡菜炒饭 虾仁炒饭 菠萝炒饭 叉烧炒饭
    番茄焖饭 土豆焖饭 南瓜饭 腊味饭 清蒸鲈鱼 清蒸鱼 酸菜鱼
    水煮鱼 红烧鱼 糖醋鱼 香煎带鱼 炸带鱼 豆豉鱼 剁椒鱼头
    油焖大虾 白灼虾 蒜蓉虾 盐焗虾 椒盐虾 醉虾 咖喱虾
    蒜蓉扇贝 烤生蚝 炒花甲 辣炒花甲 葱姜蟹 香辣蟹 蟹肉煲
    白切鸡 盐焗鸡 口水鸡 手撕鸡 椒麻鸡 葱油鸡 烤鸡 卤鸡
    烤鸭 卤鸭 姜母鸭 酱鸭 辣鸭脖 麻辣鸭脖 卤鸡爪 卤鸭掌 卤猪蹄
    白灼菜 清炒虾仁 龙井虾仁 虾仁滑蛋 蚂蚁上树 麻婆豆腐
    家常豆腐 红烧豆腐 蟹黄豆腐 皮蛋豆腐 凉拌豆腐 凉拌木耳
    凉拌海带 凉拌腐竹 凉拌粉丝 凉拌藕片 醋溜土豆 醋溜白菜
    酸辣汤 冬瓜汤 海带汤 紫菜蛋汤 番茄蛋汤 豆腐汤 菌菇汤
    排骨汤 莲藕汤 萝卜汤 鸡汤 鱼汤 鲫鱼汤 三鲜汤 玉米汤
    罗宋汤 奶油蘑菇 酸汤 蛋花汤 鱼头汤 丸子汤 牛肉汤 羊肉汤
    牛肉面 炸酱面 热干面 担担面 阳春面 葱油拌面 油泼面
    凉面 拌面 焖面 汤面 素面 麻酱面 凉皮 酸辣粉 螺蛳粉
    桂林米粉 牛肉米线 过桥米线 砂锅米线 麻辣米线 炒河粉
    干炒牛河 炒米粉 炒面 炒年糕 汤年糕 年糕汤 火鸡面 方便面
    泡面 拌粉 嗦粉 土豆粉 麻辣烫 冒菜 串串香 钵钵鸡 关东煮
    寿喜锅 部队锅 小火锅 鸳鸯锅 菌菇锅 番茄锅 清汤锅 铜锅
    潮汕牛肉 羊肉火锅 椰子鸡 猪肚鸡 石锅拌饭 烤肉 拌饭
    肉夹馍 驴肉火烧 煎饼果子 鸡蛋灌饼 煎饼卷菜 葱油饼
    酱香饼 馅饼 肉饼 千层饼 烧饼 锅盔 油饼 油条 麻花
    小笼包 生煎包 水煎包 灌汤包 肉包 菜包 豆沙包 奶黄包
    流沙包 烧卖 锅贴 煎饺 水饺 蒸饺 虾饺 馄饨 云吞 抄手
    红油抄手 酸汤水饺 鲜肉馄饨 菜肉馄饨 鸡汤馄饨 牛肉饺
    糯米烧卖 虾仁烧卖 肠粉 萝卜糕 芋头糕 马蹄糕 松糕 发糕
    红糖发糕 桂花糕 绿豆糕 豌豆黄 驴打滚 糍粑 麻团 汤圆
    元宵 酒酿圆子 银耳羹 红豆汤 绿豆汤 莲子羹 八宝粥 南瓜粥
    皮蛋瘦肉 小米粥 山药粥 海鲜粥 砂锅粥 白粥 鸡蛋粥 杂粮粥
    甜粥 咸粥 豆浆 豆花 豆腐脑 油炸糕 炸鸡 鸡米花 鸡柳
    鸡块 鸡排 猪排 牛排 鱼排 虾饼 肉串 鸡肉串 羊肉串
    牛肉串 烤肠 烤冷面 烤玉米 烤红薯 烤土豆 烤茄子 烤面筋
    薯条 薯角 薯格 薯饼 炸薯条 炸蘑菇 炸藕盒 炸茄盒 春卷
    卤味 熟食 凉菜 冷盘 拼盘 肉拼盘 水果盘 沙拉 蔬菜沙拉
    鸡肉沙拉 金枪鱼饭 三明治 帕尼尼 热狗 汉堡 披萨 薄底披萨
    蛋挞 布丁 泡芙 曲奇 玛芬 松饼 华夫饼 铜锣烧 可丽饼
    戚风蛋糕 海绵蛋糕 提拉米苏 芝士蛋糕 乳酪蛋糕 奶油蛋糕
    巴斯克 瑞士卷 蛋糕卷 千层蛋糕 舒芙蕾 雪媚娘 大福 麻薯
    肉松小贝 鲜花饼 老婆饼 桃酥 杏仁酥 蛋黄酥 月饼 冰皮月饼
    青团 粽子 蛋黄粽 鲜肉粽 蜜枣粽 豆沙粽 咸肉粽 肉粽
    牛轧糖 雪花酥 沙琪玛 夹心饼干 苏打饼干 威化饼 饼干棒
    冰淇淋 雪糕 冰棍 冰沙 刨冰 冰粉 凉粉 龟苓膏 烧仙草
    双皮奶 杨枝甘露 芒果西米 西米露 水果捞 酸奶碗 奶昔 奶茶
    珍珠奶茶 椰果奶茶 乌龙奶茶 鲜奶茶 奶盖茶 水果茶 柠檬茶
    青柠茶 冰红茶 冰绿茶 热红茶 桂花茶 菊花茶 茉莉茶 花果茶
    玄米茶 大麦茶 荞麦茶 乌龙茶 普洱茶 白茶 熟茶 生茶 抹茶
    摩卡 拿铁 美式咖啡 意式浓缩 冷萃 冰咖啡 手冲咖啡 浓缩咖啡
    燕麦拿铁 生椰拿铁 澳白 脏咖啡 黑咖啡 热巧克力 可可饮料
    鲜榨果汁 橙汁 苹果汁 西瓜汁 葡萄汁 石榴汁 椰子水 气泡水
    苏打水 柠檬水 蜂蜜水 温水 热水 凉白开 矿泉水 电解质水
    酸梅汤 乌梅汤 山楂汁 罗汉果茶 冬瓜茶 姜茶 红糖姜茶
    """),
    ("cooking_methods_and_kitchen", 6000, """
    备菜 洗菜 择菜 摘菜 切菜 配菜 备料 选料 开火 关火 生火
    大火 中火 小火 文火 武火 猛火 旺火 余温 预热 热锅 冷锅
    热油 凉油 冷油 温油 起锅 出锅 下锅 回锅 开锅 收汁 勾芡
    薄芡 浓芡 腌制 腌入味 入味 去腥 焯水 过水 沥水 控水
    沥干 擦干 焯熟 煮熟 炒熟 蒸熟 炸熟 烤熟 煮透 蒸透
    炖烂 炖软 炒软 煮软 炒散 炒匀 拌匀 揉匀 搅匀 搅散
    搅拌 打发 打匀 过筛 筛粉 扒皮 削皮 去皮 去核 去籽
    去蒂 去根 去骨 剔骨 抽筋 除刺 刮鳞 清洗 冲洗 浸泡 泡发
    泡软 泡开 泡透 解冻 化冻 冷藏 冷冻 保鲜 速冻 急冻
    腌肉 腌鱼 卤肉 卤蛋 卤菜 煎炒 蒸煮 焖煮 炖煮 烘烤
    油炸 油煎 水煮 清蒸 红烧 清炖 焖烧 干烧 干煸 油焖
    爆炒 清炒 滑炒 炒香 煸炒 慢炖 慢煮 快炒 快煮 锅气
    爆香 炸香 炒糖色 挂糊 挂浆 挂霜 拔丝 走油 滑油 复炸
    烫面 发面 和面 揉面 醒面 饧面 擀面 擀皮 抻面 拉面
    擀薄 拍扁 按扁 搓圆 搓条 捏紧 捏合 包馅 收口 折叠
    压实 压平 蒸制 烤制 炸制 煮制 卤制 熏制 风干 晾干
    烘干 脱水 冻干 烟熏 盐焗 干腌 湿腌 发酵 二发 排气
    整形 脱模 入模 模具 刷油 刷蛋液 撒盐 撒糖 撒粉 裹粉
    裹浆 粉浆 面糊 蛋液 蛋白 蛋清 蛋黄 黄油块 奶油霜
    糖霜 糖水 糖色 焦糖 翻糖 蛋奶液 酸奶油 液种 汤种
    水浴 水浴法 隔水 隔水炖 隔水蒸 低温烤 蒸汽烤 旋风烤
    低温慢煮 低温发酵 室温发酵 油温 水温 中心温度 内部温度
    计时 火候 熟度 嫩度 软硬度 八成熟 全熟 半熟 五分熟
    七分熟 三分熟 外酥里嫩 酥脆 香酥 焦脆 软糯 弹牙 筋道
    粘锅 糊锅 焦糊 焦黑 夹生 不熟 过熟 煮老 炒老 炒焦
    烤焦 烤糊 煮糊 糊底 溢锅 扑锅 油花 油烟 油渍 油垢
    浮沫 浮油 撇沫 撇油 油水 汤汁 原汤 高汤 浓汤 清汤
    鲜汤 肉汤 骨汤 汤底 锅底 汤料 料包 香料包 调料包
    留底油 干锅 平底锅 不粘锅 铸铁锅 砂锅 炖锅 奶锅 汤锅
    蒸锅 煎锅 炒锅 雪平锅 压力锅 高压锅 电压力锅 电饭锅
    电饭煲 微波炉 烤箱 蒸箱 蒸烤箱 空气炸锅 电炖锅 电蒸锅
    电煮锅 电热锅 电炒锅 电饼铛 电磁炉 燃气灶 电陶炉
    料理机 破壁机 榨汁机 搅拌机 厨师机 揉面机 和面机 豆浆机
    面包机 绞肉机 碎冰机 制冰机 咖啡机 磨豆机 热水壶 温控壶
    养生壶 保温壶 冷水壶 油壶 酱油壶 醋壶 调味罐 调料瓶
    调料盒 米桶 米缸 面粉桶 储粮桶 储物罐 密封罐 真空袋
    保鲜袋 保鲜盒 保鲜膜 食品袋 食品夹 封口夹 封口机 真空机
    厨房秤 量杯 量勺 量匙 温度计 油温计 探针 探针温度
    计时器 电子秤 刮刀 刮板 硅胶铲 木铲 锅铲 漏铲 汤勺
    汤匙 饭勺 饭铲 长柄勺 漏勺 漏网 网筛 粉筛 面粉筛
    打蛋器 蛋抽 蛋刷 油刷 毛刷 硅胶刷 擀面杖 面板 揉面垫
    硅胶垫 烘焙垫 蒸笼 蒸屉 笼屉 蒸架 蒸盘 蒸碗 蒸布
    屉布 笼布 蒸笼纸 烘焙纸 油纸 锡纸 铝箔纸 纸托 蛋糕模
    面包模 慕斯圈 烤盘 烤网 烤架 冷却架 裱花袋 裱花嘴
    裱花台 切片刀 面包刀 蛋糕刀 菜刀 砍骨刀 水果刀 削皮刀
    削皮器 擦丝器 切丝器 刨丝器 压蒜器 挤汁器 核桃夹
    开瓶器 开罐器 厨房剪 食物夹 隔热垫 隔热手套 锅盖架
    砧板 竹砧板 木砧板 塑料砧板 沥水篮 沥水架 洗菜篮
    洗菜盆 滤水篮 菜篮 食品盆 不锈钢盆 拌料盆 搅拌盆
    洗碗布 洗碗刷 洗碗棉 海绵擦 钢丝球 百洁布 厨房纸
    吸油纸 吸水纸 去油剂 洗洁精 洗碗盐 洗碗块 洗碗粉
    洗碗机 消毒柜 碗柜 碗架 置碗架 刀架 筷笼 筷子筒
    长筷 公筷 公勺 碗碟 餐盘 盘碟 汤碗 饭碗 蘸料碟
    骨碟 汤盅 砂锅盖 锅把 锅柄 锅耳 防烫夹 提碗器
    """),
    ("household_cleaning_and_storage", 5500, """
    家务 做家务 大扫除 日常清洁 深度清洁 收纳 整理 收拾 清扫
    打扫 扫地 拖地 擦地 擦桌 擦窗 擦玻璃 擦灰 除尘 掸灰
    吸尘 去污 除污 去渍 除渍 去油 除油 除垢 去垢 除霉
    去霉 除味 去味 除臭 消臭 消毒 杀菌 抑菌 防霉 防虫
    除虫 灭虫 驱虫 灭蚊 驱蚊 除螨 灭螨 除湿 防潮 防滑
    洗衣 洗鞋 洗袜 洗被套 洗枕套 洗床单 洗毛巾 晾衣 晒衣
    晾晒 晒被 晾干 风干 烘干 甩干 脱水 漂洗 浸洗 浸泡
    搓洗 揉搓 刷洗 机洗 手洗 干洗 水洗 预洗 轻柔洗 快洗
    羊毛洗 混合洗 洗烘一体 洗衣液 洗衣粉 洗衣凝珠 洗衣皂
    肥皂皂盒 肥皂盒 皂盒 香皂盒 柔顺剂 留香珠 衣物柔顺
    去渍笔 去渍剂 除菌液 衣物消毒 漂白剂 彩漂 氧漂 氯漂
    增白剂 除垢剂 清洁剂 清洁液 清洁膏 清洁泡沫 清洁喷雾
    玻璃水 玻璃清洁 地板清洁 马桶清洁 水垢清洁 油污清洁
    管道疏通 疏通剂 洗衣机槽 水槽滤网 水槽塞 地漏芯 地漏盖
    下水管 排水管 进水管 软管 延长管 三通 弯头 管接头
    水龙头 水嘴 花洒 花洒头 花洒软管 淋浴杆 升降杆 挂墙架
    免打孔 免钉胶 玻璃胶 密封胶 美缝剂 填缝剂 胶枪 胶带
    生料带 防水胶带 双面胶带 泡棉胶 挂钩胶 无痕胶 魔术贴
    扎带 理线带 理线夹 理线盒 收线盒 电线夹 网线夹 插座盒
    防尘塞 防尘盖 防尘罩 防尘袋 防虫网 纱窗网 纱门 门帘
    遮光帘 透光帘 窗纱 百叶帘 卷帘 帘杆 窗帘环 窗帘钩
    门挡 门吸 门碰 门缓冲 防撞条 防撞角 密封条 门底条
    窗缝条 隔音条 毛条 防水条 防滑垫 防滑贴 止滑垫 地垫
    入户垫 浴室垫 厨房垫 瑜伽垫 椅垫 凳垫 坐垫 靠垫 抱枕
    抱枕套 枕芯 枕套 枕巾 床罩 床笠 床单 被套 被芯 夏凉被
    空调被 蚕丝被 羽绒被 毛毯 绒毯 午睡毯 法兰绒 珊瑚绒
    睡袋 蚊帐 蚊帐架 床围 床栏 床边桌 床头柜 斗柜 衣柜
    挂衣杆 衣架 裤架 裙架 袜夹 晒衣夹 晾衣杆 晾衣架 晒衣架
    落地衣架 挂衣架 折叠衣架 内衣架 伸缩杆 置物杆 衣帽架
    脏衣篮 洗衣篮 洗衣袋 护洗袋 网洗袋 内衣袋 收纳袋 压缩袋
    真空压缩 旅行收纳 分装瓶 分装罐 分装袋 试用装 小样 便携装
    收纳盒 抽屉盒 桌面收纳 抽屉收纳 衣物收纳 鞋盒 鞋架 鞋柜
    鞋撑 鞋拔 鞋刷 鞋擦 鞋油 防水喷雾 除臭鞋垫 鞋垫 烘鞋器
    收纳筐 收纳篮 储物箱 储物柜 储物架 置物架 置物篮 置物筐
    置物板 层板 搁板 壁挂架 洞洞板 隔板 分隔板 书挡 书立
    挂篮 挂袋 袋中袋 文件篮 文件袋 票据袋 零钱袋 零钱包
    钥匙盘 钥匙盒 钥匙挂钩 遥控器架 手机支架 平板支架 充电架
    滚轮 万向轮 滑轮 防尘轮 轮脚 支脚 桌脚 椅脚 调节脚
    地板保护 桌脚垫 椅脚垫 硅胶垫 毛毡垫 防滑脚垫 软脚垫
    手动扫把 扫帚 簸箕 垃圾铲 畚箕 拖把 平板拖把 旋转拖把
    拖把桶 拖把头 拖把布 扫地机 拖地机 洗地机 吸尘器 除螨仪
    吸尘袋 集尘袋 尘盒 尘袋 尘杯 滤芯 滤网 滤棉 过滤棉
    过滤网 滤尘网 滤尘袋 静电拖把 静电除尘 除尘纸 除尘掸
    鸡毛掸 除尘刷 缝隙刷 马桶刷 长柄刷 硬毛刷 软毛刷 清洁刷
    刮水器 刮水板 玻璃刮 玻璃擦 擦窗器 磁力擦 抹布 超细纤维
    清洁布 擦拭布 吸水布 方巾 浴巾 毛巾 毛巾架 毛巾杆
    浴巾架 纸巾盒 抽纸盒 纸巾架 卷纸架 厕纸盒 厕纸架
    垃圾桶 垃圾篓 垃圾袋 垃圾夹 厨余桶 干垃圾 湿垃圾 可回收
    有害垃圾 易腐垃圾 垃圾分类 垃圾投放 垃圾清运 垃圾打包
    厨余垃圾 垃圾处理 蚊香 电蚊香 蚊香液 蚊香片 电蚊拍
    驱蚊液 驱蚊贴 防蚊液 杀虫剂 蟑螂药 蟑螂屋 诱虫灯
    净水器 饮水机 直饮机 前置过滤 软水机 滤水壶 净水壶
    加湿器 除湿机 除湿盒 除湿袋 干燥剂 竹炭包 活性炭 空气净化
    香薰 香薰机 香薰灯 扩香石 扩香瓶 无火香薰 香薰蜡烛
    电暖器 暖风机 踢脚线炉 油汀 电热毯 暖手宝 热水袋 凉席
    竹席 冰丝席 凉感被 透气垫 通风扇 排风扇 换气扇 循环扇
    塔扇 壁扇 台扇 落地扇 风扇罩 遥控风扇 夹扇 挂脖风扇
    脚轮 软胶脚垫 洗衣网袋 衣物护理
    """),
    ("nature_gardening_and_landscape", 5500, """
    花草 草木 绿植 盆栽 盆景 花盆 花架 花托 托盘 花土 营养土
    泥炭土 椰糠 珍珠岩 蛭石 陶粒 赤玉土 腐叶土 园土 沙土
    颗粒土 水培 土培 多肉 多肉植物 观叶植物 观花植物 耐阴植物
    喜阳植物 草本 木本 藤本 藤蔓 灌木 乔木 宿根 一年生 多年生
    种子 花种 菜种 种苗 幼苗 小苗 枝条 枝梢 枝杈 枝干 枝叶
    叶尖 叶缘 叶片 叶脉 叶柄 叶芽 花芽 芽点 芽眼 芽苞 花苞
    花蕾 花瓣 花萼 花蕊 雄蕊 雌蕊 花茎 花梗 花序 果实 果肉
    果核 果皮 果柄 果蒂 瓜藤 瓜蔓 果藤 树皮 树干 树枝 树冠
    树荫 林荫 枯枝 枯叶 落叶 落花 黄叶 绿叶 嫩叶 新叶 老叶
    卷叶 焦叶 焦边 烂根 根须 根系 主根 侧根 须根 气根 根茎
    鳞茎 球茎 块茎 块根 匍匐茎 发芽 出芽 萌芽 冒芽 抽芽
    抽枝 抽叶 长叶 长根 开花 结蕾 结花 结果 挂果 坐果 结籽
    授粉 传粉 花粉 花蜜 花期 果期 盛花期 休眠期 生长期 萌发
    播种 育苗 育种 移栽 定植 扦插 压条 嫁接 分株 换盆 翻盆
    修根 修枝 剪枝 剪叶 打顶 摘心 疏花 疏果 疏枝 疏叶
    浇水 喷水 洒水 淋水 控水 沥水 滴灌 喷灌 浇透 湿润
    保湿 保水 补水 排水 透水 透气 积水 缺水 干透 半干 微湿
    干湿 湿度 光照 日照 散射光 直射光 半日照 全日照 背阴 朝阳
    向阳 遮阴 遮阳 遮阳网 防晒网 补光灯 植物灯 护根 冻根
    施肥 追肥 底肥 基肥 叶面肥 有机肥 复合肥 缓释肥 水溶肥
    营养液 生根粉 杀菌剂 殺虫剂 叶斑病 白粉病 蚜虫 红蜘蛛
    介壳虫 小黑飞 飞虱 蓟马 蚧壳虫 地老虎 蛴螬 菜青虫
    月季 玫瑰 蔷薇 绣球 木绣球 三角梅 茉莉 栀子 米兰 桂花
    海棠 山茶 茶花 杜鹃 映山红 牡丹 芍药 腊梅 梅花 桃花
    梨花 杏花 樱花 玉兰 白玉兰 紫玉兰 紫薇 木槿 芙蓉 紫荆
    紫藤 凌霄 金银花 迎春花 连翘 丁香花 白丁香 牵牛花 矮牵牛
    太阳花 长寿花 蟹爪兰 君子兰 蝴蝶兰 石斛兰 文心兰 建兰
    墨兰 春兰 兰花 菊花 雏菊 玛格丽特 百日草 波斯菊 金盏菊
    万寿菊 瓜叶菊 非洲菊 大丽花 朱顶红 郁金香 风信子 水仙
    小苍兰 鸢尾 马蹄莲 美人蕉 百合花 萱草 六出花 洋桔梗
    桔梗 康乃馨 满天星 勿忘我 飞燕草 毛地黄 金鱼草 石竹
    凤仙花 玻璃翠 天竺葵 花毛茛 银莲花 马鞭草 薰衣草 鼠尾草
    千日红 鸡冠花 千日粉 虞美人 花菱草 福禄考 蜀葵 旱金莲
    绿萝 吊兰 牡丹吊兰 常春藤 合果芋 龟背竹 春羽 天堂鸟
    散尾葵 富贵竹 幸福树 发财树 金钱树 橡皮树 琴叶榕 鸭脚木
    铜钱草 虎皮兰 龙血树 朱蕉 红掌 白掌 一帆风顺 铁线蕨
    波士顿蕨 鹿角蕨 鸟巢蕨 文竹 武竹 南天竹 千年木 豆瓣绿
    镜面草 冷水花 竹芋 孔雀竹芋 白网纹草 彩叶草 变叶木
    芦荟 仙人掌 仙人球 仙人指 昙花 令箭荷花 佛珠 情人泪
    玉露 桃蛋 熊童子 虹之玉 姬胧月 乙女心 黑法师 玉树
    金枝玉叶 马齿苋 不死鸟 长生草 生石花 十二卷 景天 铁筷子
    柳树 杨树 榆树 槐树 白蜡 银杏 梧桐 枫树 红枫 乌桕
    水杉 落羽杉 雪松 黑松 油松 罗汉松 侧柏 龙柏 香樟 广玉兰
    榕树 木棉 凤凰木 合欢 紫叶李 石楠 红叶石楠 黄杨 小叶黄杨
    女贞 冬青 海桐 柏树 杉树 桦树 白桦 胡杨 沙柳 柽柳
    苔藓 青苔 地衣 蘑菇 菌丝 孢子 真菌 蕨类 水草 荷花 睡莲
    芦苇 蒲草 蒲苇 狗尾草 芒草 细叶芒 狼尾草 浮萍 水葫芦
    蒲公英 车前草 酢浆草 三叶草 牛筋草 蓬草 雀麦 艾草 蒿草
    春风 夏风 秋风 冬风 清风 微风 阵风 凉风 暖风 海风 山风
    晨风 晚风 夜风 东风 西风 南风 北风 逆风 顺风 顶风 迎风
    春雨 夏雨 秋雨 冬雨 细雨 微雨 小雨 阵雨 雷雨 暴雨 骤雨
    梅雨 夜雨 山雨 喜雨 霏霏 淅沥 雨丝 雨滴 雨点 雨幕 雨雾
    雨声 雨脚 雨势 雨意 雨痕 雨后 雨前 雨中 风雨 晴雨 云雨
    朝露 晨露 露珠 露水 白露 寒露 霜花 霜冻 晨霜 薄霜
    冰霜 冰花 冰凌 冰挂 冰柱 冰面 薄冰 浮冰 碎冰 流冰
    雪花 雪片 雪粒 雪沫 雪粉 雪幕 雪景 雪原 雪坡 雪峰
    雪线 雪山 雪地 积雪 残雪 初雪 瑞雪 大雪 飞雪 落雪 飘雪
    阴云 乌云 白云 彩云 浮云 流云 云朵 云层 云团 云海 云霞
    晚霞 朝霞 霞光 霞色 红霞 彩霞 浓雾 薄雾 轻雾 晨雾
    山雾 水雾 烟雾 雾气 雾霭 岚气 山岚 烟岚 云岚 迷雾
    晨光 朝阳 初阳 旭日 夕阳 斜阳 落日 余晖 暮光 暮色
    月光 月色 月影 月辉 月华 清辉 明月 新月 残月 弯月
    皓月 满月 圆月 缺月 月牙 星光 星辉 星河 星空 星辰
    星斗 繁星 群星 流星 银河 天河 斗转 北斗 长空 苍穹
    河滩 河岸 河湾 河道 河床 河口 河源 河谷 河畔 水岸
    江岸 江畔 江面 江水 江流 江潮 江滩 湖岸 湖畔 湖面
    湖水 湖心 湖湾 湖光 湖色 山色 水色 波光 波纹 波浪
    细浪 微澜 涟漪 碧波 碧浪 浪花 浪涛 海浪 海面 海岸
    海湾 海滩 海岬 海岛 孤岛 小岛 岛礁 沙滩 沙洲 沙丘
    沙地 沙土 沙砾 砂砾 细沙 河沙 流沙 潮水 潮汐 涨潮
    退潮 满潮 低潮 海潮 暗流 激流 水流 溪流 山溪 清溪
    小溪 溪谷 溪涧 溪畔 泉水 山泉 清泉 温泉 泉眼 泉源
    泉声 瀑布 飞瀑 水潭 深潭 浅滩 深涧 山涧 沟壑 山壑
    山谷 峡谷 山坳 山坡 山脊 山岭 山峦 山巅 山顶 山脚
    山麓 山腰 山路 山径 山野 山村 山林 密林 树林 林地
    林间 林下 林海 林木 林梢 林冠 林缘 竹林 竹海 竹丛
    草地 草坪 草丛 草甸 草原 旷野 荒野 沃野 田野 原野
    麦田 稻田 菜地 菜畦 果园 茶园 桑园 橘园 梨园 桃园
    """),
    ("description_literary_and_reusable_phrases", 6000, """
    松软 绵软 酥软 柔软 软和 软乎 软绵绵 硬邦邦 软塌塌 软趴趴
    蓬松 毛茸茸 毛绒绒 湿漉漉 干巴巴 热乎乎 暖乎乎 冷冰冰
    热腾腾 香喷喷 甜丝丝 酸溜溜 辣乎乎 油腻腻 黏糊糊 粘乎乎
    粘稠 黏稠 稀薄 浓稠 浓厚 稠密 疏密 疏松 紧实 结实
    嫩滑 滑嫩 爽脆 清脆 脆嫩 鲜嫩 鲜甜 鲜香 浓香 醇香
    醇厚 浓郁 淡雅 清雅 清冽 凛冽 温润 湿润 干涩 枯涩
    圆润 饱满 丰满 干瘪 瘪瘦 肥厚 厚实 单薄 轻薄 厚重
    笨重 轻盈 轻巧 轻快 轻捷 灵巧 灵动 灵便 清透 通透
    透亮 明亮 光亮 鲜亮 锃亮 乌亮 油亮 发亮 闪亮 闪耀
    柔和 柔美 柔婉 柔顺 柔韧 柔媚 柔弱 柔嫩 柔情 柔软
    细腻 细嫩 细碎 细微 细致 精细 精致 精巧 精美 简约
    素净 素雅 古朴 质朴 朴素 简朴 淳朴 纯朴 敦厚 温厚
    浑厚 深厚 雄厚 雄浑 浑浊 混浊 清浊 清澈 澄澈 澄明
    明净 洁净 干净 清净 清幽 幽静 寂静 静谧 宁静 安静
    娴静 恬静 恬淡 恬然 悠然 安然 泰然 淡然 坦然 自若
    安稳 稳妥 稳当 妥当 牢靠 可靠 牢固 稳固 稳健 稳重
    平稳 平缓 缓慢 缓和 舒缓 和缓 悠缓 悠长 绵长 漫长
    冗长 简短 短促 急促 急切 急躁 匆促 仓促 仓皇 慌张
    惶然 茫然 怅然 悵惘 怅惘 迷惘 惘然 失落 低落 沮丧
    颓唐 颓然 消沉 萧索 萧瑟 凄清 凄冷 凄凉 冷清 冷寂
    清冷 寂寥 寥落 孤寂 孤单 孤独 落寞 寂寞 沉寂 沉静
    孤清 清寒 清苦 清贫 困窘 窘迫 狼狈 局促 拘谨 拘束
    腼腆 羞赧 羞涩 羞怯 胆怯 畏缩 退缩 迟疑 犹豫 踌躇
    迟滞 滞涩 呆滞 僵直 僵硬 生硬 生涩 艰涩 晦涩 隐晦
    模糊 朦胧 隐约 隐隐 若隐若现 依稀 恍惚 恍然 赫然
    蓦然 猝然 突然 陡然 骤然 骤变 巨变 变幻 变迁 流转
    轮转 流逝 消逝 远逝 飘逝 淡去 散去 退去 渐远 渐近
    渐浓 渐淡 渐暗 渐明 微明 微亮 微暗 微弱 微小 渺小
    苍茫 莽苍 浩渺 苍穹 浩瀚 宽广 广阔 辽阔 寥廓 空阔
    开阔 宽阔 旷远 悠远 高远 深远 深邃 幽邃 邃远 渺远
    绵延 连绵 延绵 蜿蜒 曲折 逶迤 迤逦 盘曲 盘旋 回环
    环绕 缭绕 萦绕 萦回 回旋 回荡 飘荡 漂荡 漂流 飘流
    浮动 浮荡 浮沉 浮泛 浮现 涌现 显现 呈现 隐现 隐没
    淹没 湮没 吞没 没入 浸没 浸润 渗透 渗入 浸透 透出
    透过 穿透 投下 洒下 撒下 铺开 铺满 覆盖 覆满 遍布
    布满 散布 星罗棋布 错落有致 鳞次栉比 参差不齐 高低错落
    纵横交错 盘根错节 千姿百态 形态各异 婀娜多姿 摇曳多姿
    清风徐来 微风拂面 春风和煦 春光明媚 春意盎然 春暖花开
    春色满园 鸟语花香 草长莺飞 花团锦簇 繁花似锦 百花争艳
    绿树成荫 绿意盎然 枝繁叶茂 郁郁葱葱 生机勃勃 欣欣向荣
    瓜果飘香 稻谷飘香 金风送爽 秋高气爽 天高云淡 风和日丽
    晴空万里 万里无云 阳光明媚 阳光灿烂 日照充足 晨曦微露
    朝霞满天 夕阳西下 落日余晖 暮色四合 华灯初上 夜色渐浓
    月明星稀 月色如水 月朗风清 清风明月 星光点点 繁星点点
    夜深人静 万籁俱寂 鸦雀无声 悄无声息 寂然无声 静默无言
    细雨蒙蒙 烟雨蒙蒙 细雨绵绵 淅淅沥沥 雨过天晴 雨后初霁
    云开雾散 烟消云散 云雾缭绕 雾气腾腾 云蒸霞蔚 烟波浩渺
    水天一色 波光粼粼 碧波荡漾 流水潺潺 泉水叮咚 清澈见底
    一泓清水 一湾碧水 山清水秀 青山绿水 崇山峻岭 连绵起伏
    层峦叠嶂 重峦叠嶂 悬崖峭壁 奇峰怪石 苍翠欲滴 竹影婆娑
    白雪皑皑 银装素裹 冰天雪地 大雪纷飞 雪花飞舞 寒风凛冽
    风雪交加 漫天飞雪 滴水成冰 寒气逼人 暖意融融 暖意洋洋
    香气扑鼻 香气四溢 香味浓郁 色泽诱人 色香味全 入口即化
    回味无穷 唇齿留香 食欲大增 酸甜可口 鲜嫩多汁 软糯香甜
    外焦里嫩 外脆里软 清爽可口 口感细腻 口感丰富 饱满多汁
    家常便饭 粗茶淡饭 三餐四季 人间烟火 烟火气 市井气 生活气息
    忙里偷闲 闲情逸致 闲庭信步 信步闲游 散步闲聊 走走停停
    说说笑笑 谈谈笑笑 断断续续 陆陆续续 慢慢悠悠 晃晃悠悠
    摇摇晃晃 蹒蹒跚跚 跌跌撞撞 急急忙忙 匆匆忙忙 风风火火
    从从容容 不慌不忙 不紧不慢 有条不紊 井然有序 顺手一放
    随手一放 随手一拿 顺手一拿 轻轻一推 轻轻一拉 轻轻一拍
    轻轻一按 轻轻一碰 微微一笑 淡淡一笑 会心一笑 嫣然一笑
    莞尔一笑 付之一笑 粲然一笑 哑然失笑 忍俊不禁 喜不自胜
    心花怒放 满心欢喜 心生欢喜 心怀感激 心存感激 满怀期待
    若有所思 若有所失 怅然若失 如释重负 恍然大悟 豁然开朗
    不知不觉 浑然不觉 悄然无声 悄然离去 悄然而至 蓦然回首
    依依不舍 恋恋不舍 流连忘返 念念不忘 心心念念 久久不忘
    历历在目 记忆犹新 铭记于心 铭心刻骨 忆苦思甜 感慨万千
    百感交集 五味杂陈 思绪万千 思绪纷飞 心绪不宁 心神不定
    心潮起伏 心潮澎湃 心驰神往 心旷神怡 悠然自得 怡然自得
    自得其乐 安然自若 泰然自若 淡然处之 坦然面对 安之若素
    宠辱不惊 随遇而安 顺其自然 水到渠成 日积月累 细水长流
    日复一日 年复一年 日久天长 旷日持久 经年累月 岁月静好
    时光流转 岁月流逝 光阴荏苒 时过境迁 今非昔比 物是人非
    世事变迁 人来人往 车来车往 熙熙攘攘 熙来攘往 川流不息
    络绎不绝 纷至沓来 接踵而至 摩肩接踵 门庭若市 热闹非凡
    热火朝天 欢声笑语 笑语盈盈 其乐融融 一派祥和 满目疮痍
    零零落落 稀稀落落 三三两两 斑斑点点 点点滴滴 丝丝缕缕
    片片花瓣 缕缕炊烟 淡淡清香 微微凉意 阵阵花香 隐隐作痛
    不由自主 情不自禁 不禁感叹 不胜感慨 禁不住 忍不住 耐不住
    熬不住 挨不住 撑不住 放不下 舍不得 放不开 看不开 想不开
    来不及 赶不及 等不及 急不得 慢不得 拖不得 耽搁不得
    拿不准 说不准 说不清 记不清 看不清 分不清 猜不透 想不通
    理不清 讲不明 讲不清 弄不懂 摸不透 算不出 看不懂 听不懂
    做不到 办不到 想不到 看不到 找不到 碰不到 等不到 到不了
    回不去 过不去 进不去 出不去 上不去 下不去 放不进去
    带不走 搬不动 拿不动 推不动 拉不动 走不动 跑不动 爬不动
    """),
]

# Compound pronunciations reviewed by meaning; joined syllables are runtime
# dictionary keys. This map is intentionally readable as an audit checklist.
OVERRIDES = {
    "调味料": "tiaoweiliao", "复合调料": "fuhetiaoliao",
    "调料包": "tiaoliaobao", "调料瓶": "tiaoliaoping",
    "调料盒": "tiaoliaohe", "调味罐": "tiaoweiguan",
    "调节脚": "tiaojiejiao", "朝天椒": "chaotianjiao",
    "黑芝麻粉": "heizhimafen", "饱腹感": "baofugan",
    "枸杞": "gouqi", "巴旦木": "badanmu", "马苏里拉": "masulila",
    "干豆腐": "gandoufu", "豆干": "dougan", "香干": "xianggan",
    "干木耳": "ganmuer", "干香菇": "ganxianggu",
    "鲜香菇": "xianxianggu", "干辣椒": "ganlajiao",
    "长茄": "changqie", "茄子": "qiezi", "茄盒": "qiehe",
    "鱼香茄子": "yuxiangqiezi", "肉末茄子": "roumoqiezi",
    "蒜蓉茄子": "suanrongqiezi", "红烧茄子": "hongshaoqiezi",
    "炸茄盒": "zhaqiehe", "烤茄子": "kaoqiezi",
    "番茄酱": "fanqiejiang", "番茄": "fanqie", "番茄炒蛋": "fanqiechaodan",
    "番茄焖饭": "fanqiemenfan", "番茄蛋汤": "fanqiedantang", "番茄锅": "fanqieguo",
    "红薯粉": "hongshufen", "绿豆粉": "lvdoufen", "木薯粉": "mushufen",
    "粘米粉": "zhanmifen", "面粉": "mianfen", "生粉": "shengfen",
    "米粉": "mifen", "藕粉": "oufen", "花椒": "huajiao",
    "豆豉": "douchi", "蒸鱼豉油": "zhengyuchiyou", "豆豉鱼": "douchiyu",
    "豇豆": "jiangdou", "芫荽": "yansui", "苋菜": "xiancai",
    "荸荠": "biqi", "茭白": "jiaobai", "腐竹": "fuzhu",
    "百合": "baihe", "秋葵": "qiukui", "燕麦": "yanmai",
    "藜麦": "limai", "粳米": "jingmi", "豆乳": "douru",
    "鲅鱼": "bayu", "鲳鱼": "changyu", "蛤蜊": "geli", "蛏子": "chengzi",
    "花蛤": "huage", "鹌鹑蛋": "anchundan", "鳕鱼": "xueyu",
    "扇贝": "shanbei", "贝柱": "beizhu", "干贝": "ganbei",
    "黄喉": "huanghou", "牛百叶": "niubaiye", "毛肚": "maodu",
    "火锅底料": "huoguodiliao", "白切鸡": "baiqieji",
    "白灼虾": "baizhuoxia", "白灼菜": "baizhuocai", "龙井虾仁": "longjingxiaren",
    "油炸": "youzha", "炸带鱼": "zhadaiyu", "炸薯条": "zhashutiao",
    "炸蘑菇": "zhamogu", "炸藕盒": "zhaouhe", "油炸糕": "youzhagao",
    "炸鸡": "zhaji", "炸香": "zhaxiang", "炸熟": "zhashu",
    "炸制": "zhazhi", "复炸": "fuzha", "炸酱面": "zhajiangmian",
    "空气炸锅": "kongqizhaguo", "干煸": "ganbian", "煸炒": "bianchao",
    "干煸豆角": "ganbiandoujiao", "豆角": "doujiao", "牛角包": "niujiaobao",
    "凉皮": "liangpi", "凉面": "liangmian", "凉粉": "liangfen",
    "嗦粉": "suofen", "螺蛳粉": "luosifen", "抄手": "chaoshou",
    "红油抄手": "hongyouchaoshou", "钵钵鸡": "boboji",
    "驴打滚": "lvdagun", "和面": "huomian", "和面机": "huomianji",
    "烙饼": "laobing", "烫面": "tangmian", "擀面": "ganmian",
    "抻面": "chenmian", "饧面": "xingmian", "擀面杖": "ganmianzhang",
    "长柄勺": "changbingshao", "长筷": "changkuai", "长柄刷": "changbingshua",
    "薄芡": "baoqian", "擀薄": "ganbao", "薄底披萨": "baodipisa",
    "勾芡": "gouqian", "打发": "dafa", "露水": "lushui",
    "熟食": "shushi", "熟度": "shudu", "八成熟": "bachengshu",
    "半熟": "banshu", "全熟": "quanshu", "熟茶": "shucha",
    "三分熟": "sanfenshu", "五分熟": "wufenshu", "七分熟": "qifenshu",
    "发酵": "fajiao", "二发": "erfa", "发面": "famian", "发糕": "fagao",
    "红糖发糕": "hongtangfagao", "低温发酵": "diwenfajiao",
    "室温发酵": "shiwenfajiao", "结实": "jieshi", "结籽": "jiezi",
    "结果": "jieguo", "结花": "jiehua", "结蕾": "jielei", "长叶": "zhangye",
    "长根": "zhanggen", "长寿花": "changshouhua", "长生草": "changshengcao",
    "生长期": "shengzhangqi", "草长莺飞": "caozhangyingfei",
    "植物灯": "zhiwudeng", "侧柏": "cebai", "卷叶": "juanye",
    "卷心菜": "juanxincai", "卷帘": "juanlian", "朝露": "zhaolu",
    "朝阳": "zhaoyang", "初阳": "chuyang", "朝霞": "zhaoxia",
    "朝霞满天": "zhaoxiamantian", "朝天": "chaotian",
    "喜阳植物": "xiyangzhiwu", "黄叶": "huangye", "落叶": "luoye",
    "波斯菊": "bosiju", "石竹": "shizhu", "石楠": "shinan",
    "石斛兰": "shihulan", "旱金莲": "hanjinlian", "郁金香": "yujinxiang",
    "紫叶李": "ziyeli", "红叶石楠": "hongyeshinan", "木槿": "mujin",
    "芍药": "shaoyao", "荨麻": "xunma", "莴笋": "wosun",
    "铁线蕨": "tiexianjue", "蕨类": "juelei", "虞美人": "yumeiren",
    "颓唐": "tuitang", "怅惘": "changwang", "踌躇": "chouchu",
    "惘然": "wangran", "腼腆": "miantian", "羞赧": "xiunan",
    "氤氲": "yinyun", "迤逦": "yili", "逶迤": "weiyi",
    "静谧": "jingmi", "寂寥": "jiliao", "凛冽": "linlie",
    "清冽": "qinglie", "磅礴": "pangbo", "参差不齐": "cencibuqi",
    "鳞次栉比": "lincizhibi", "重峦叠嶂": "chongluandiezhang",
    "层峦叠嶂": "cengluandiezhang", "竹影婆娑": "zhuyingposuo",
    "薄霜": "boshuang", "薄冰": "bobing", "薄雾": "bowu",
    "单薄": "danbo", "轻薄": "qingbo", "稀薄": "xibo",
    "莞尔一笑": "waneryixiao", "粲然一笑": "canranyixiao",
    "忍俊不禁": "renjunbujin", "喜不自胜": "xibuzisheng",
    "禁不住": "jinbuzhu", "情不自禁": "qingbuzijin",
    "不禁感叹": "bujingantan", "不胜感慨": "bushenggankai",
    "舍不得": "shebude", "急不得": "jibude",
    "慢不得": "manbude", "拖不得": "tuobude", "耽搁不得": "dangebude",
    "夜深人静": "yeshenrenjing", "长空": "changkong", "悠长": "youchang",
    "绵长": "mianchang", "漫长": "manchang", "冗长": "rongchang",
    "旷日持久": "kuangrichijiu", "日久天长": "rijiutianchang",
    "熙熙攘攘": "xixirangrang", "熙来攘往": "xilairangwang",
    "纷至沓来": "fenzhitalai", "接踵而至": "jiezhongerzhi",
    "斑斑点点": "banbandiandian", "茉莉茶": "molicha",
    "沥干": "ligan", "擦干": "cagan", "干透": "gantou", "半干": "bangan",
    "干锅": "ganguo", "干腌": "ganyan", "风干": "fenggan", "晾干": "lianggan",
    "烘干": "honggan", "干湿": "ganshi", "干垃圾": "ganlaji",
    "干燥剂": "ganzaoji", "干巴巴": "ganbaba", "干瘪": "ganbie",
    "干净": "ganjing", "朱顶红": "zhudinghong", "昙花": "tanhua",
    "硅胶垫": "guijiaodian", "鸟巢蕨": "niaochaojue",
    "鸡爪": "jizhua", "卤鸡爪": "lujizhua", "蟹爪兰": "xiezhualan",
    "肋排": "leipai", "圆茄": "yuanqie", "味醂": "weilin",
    "牛轧糖": "niugatang", "粘乎乎": "nianhuhu",
    "蛋糕模": "dangaomu", "面包模": "mianbaomu",
    "古朴": "gupu", "质朴": "zhipu", "纯朴": "chunpu",
}

# Atypical/incorrect forms noticed in the authored draft are excluded before
# output, rather than silently admitted as plausible sounding vocabulary.
EXCLUDED = {
    "殺虫剂", "悵惘", "千日粉", "奶油蘑菇", "金枪鱼饭", "芒果西米",
    "皮蛋瘦肉", "肥皂皂盒", "门缓冲", "防尘轮", "轮脚", "网洗袋",
    "软脚垫", "衣物柔顺", "手动扫把", "结花", "辣鸭脖",
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def baseline_rows():
    paths = [ROOT / "resources/dictionary" / name for name in
             ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")]
    paths += [OUT / "lexicon_batch_001.tsv", OUT / "lexicon_batch_002.tsv"]
    keys, words = set(), set()
    for path in paths:
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) != 3:
                raise ValueError(f"Malformed baseline row in {path.name}: {line}")
            keys.add((parts[0], parts[1]))
            words.add(parts[1])
    return paths, keys, words


def main():
    paths, existing_keys, existing_words = baseline_rows()
    accepted, seen, repeated, alternate = [], set(), 0, []
    candidate_count = 0
    for category, weight, raw in SECTIONS:
        for word in raw.split():
            candidate_count += 1
            if word in EXCLUDED or word in seen:
                repeated += 1
                continue
            seen.add(word)
            if not re.fullmatch(r"[\u4e00-\u9fff]{2,4}", word):
                # Batch003 keeps only compact 2--4 character vocabulary.
                continue
            pinyin = OVERRIDES.get(word) or "".join(lazy_pinyin(word, style=Style.NORMAL))
            if not re.fullmatch("[a-z]+", pinyin):
                raise ValueError(f"Invalid Pinyin for {word}: {pinyin}")
            if (pinyin, word) in existing_keys:
                continue
            if word in existing_words:
                alternate.append({"word": word, "proposed_pinyin": pinyin})
                # A pronunciation correction belongs in a separate review;
                # batch003 is strictly genuinely new vocabulary.
                continue
            accepted.append((category, pinyin, word, weight))
    accepted.sort(key=lambda row: (row[0], row[1], row[2]))
    lines, previous_category = [], None
    for category, pinyin, word, weight in accepted:
        if category != previous_category:
            lines.append(f"# category: {category}")
            previous_category = category
        lines.append(f"{pinyin}\t{word}\t{weight}")
    target = OUT / "lexicon_batch_003.tsv"
    target.write_text("\n".join(lines) + "\n", encoding="utf-8")
    data = {
        "batch": "lexicon_batch_003", "authoring_source": "first_party_authored",
        "pronunciation_tool": "pypinyin authoring-only; explicit meaning-reviewed overrides",
        "external_lexicon_import": False, "authored_candidates": candidate_count,
        "new_entries": len(accepted),
        "category_counts": dict(sorted(Counter(row[0] for row in accepted).items())),
        "length_distribution": dict(sorted(Counter(str(len(row[2])) for row in accepted).items())),
        "frequency_range": [min(row[3] for row in accepted), max(row[3] for row in accepted)],
        "sha256": sha256(target),
        "dedup_baseline_sha256": {p.name: sha256(p) for p in paths if p.exists()},
        "dedup_policy": "Exclude existing exact keys and existing words with alternate generated pronunciations",
        "alternate_pronunciation_candidates_excluded": alternate,
        "manual_draft_corrections": {
            "鸡爪": ["jizhao", "jizhua"], "卤鸡爪": ["lujizhao", "lujizhua"],
            "蟹爪兰": ["xiezhaolan", "xiezhualan"], "肋排": ["lepai", "leipai"],
            "圆茄": ["yuanjia", "yuanqie"], "味醂": ["weilan", "weilin"],
            "牛轧糖": ["niuyatang", "niugatang"], "粘乎乎": ["zhanhuhu", "nianhuhu"],
            "蛋糕模": ["dangaomo", "dangaomu"], "面包模": ["mianbaomo", "mianbaomu"],
        },
        "pronunciation_review_notes": "Mainland everyday chicken-claw and rib readings; nominal baking mould mu; industrial verb forms 入模/脱模 retain mo. 粘乎乎 is sticky adjective nian; 粘锅 is adhesive verb zhan. This is a contributor review, not exhaustive linguistic certification.",
        "pronunciation_check_references": [
            "https://pedia.cloud.edu.tw/Entry/Detail?title=%E9%86%82",
            "https://dict.revised.moe.edu.tw/dictView.jsp?ID=3260&la=1&powerMode=0",
            "https://dict.mini.moe.edu.tw/SearchIndex/word_detail?breadcrumbs=Search_%E7%B2%98_one&dictSearchField=%E7%B2%98&wordID=D0000297",
        ],
        "canonical_dictionaries_modified": False,
    }
    (OUT / "lexicon_batch_003.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in data.items() if k not in ("dedup_baseline_sha256", "alternate_pronunciation_candidates_excluded")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
