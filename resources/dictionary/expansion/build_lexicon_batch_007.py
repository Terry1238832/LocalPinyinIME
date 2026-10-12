#!/usr/bin/env python3
"""Selected missing household, communication, sports, services and time terms."""
from __future__ import annotations
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
from pypinyin import Style, lazy_pinyin, pinyin

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent

# Individually selected reusable terms after surveying current category coverage.
# No imported word list, Cartesian product, article or benchmark answer injection.
SECTIONS = [
    ("household_hardware_and_hand_tools", 5500, """
    五金 五金工具 家用工具 工具箱 工具包 工具盒 工具套装 五金配件 安装工具
    螺丝刀 起子 螺丝批 电动起子 电动工具 手动工具 充电钻 电钻 手电钻
    冲击钻 电锤 电镐 电批 电动扳手 电动螺丝 气动工具 气动扳手 气动钉枪
    扭力扳手 力矩扳手 开口扳手 梅花扳手 棘轮扳手 套筒扳手 套筒 六角套筒
    六角扳手 内六角 外六角 六角匙 扳手套装 十字批头 一字批头 六角批头
    磁性批头 磁性螺丝 加长批头 批头 延长杆 批头套装 钻头 麻花钻
    木工钻 金属钻 水泥钻 瓷砖钻 玻璃钻 开孔器 开孔钻 阶梯钻 扩孔钻
    螺丝规格 螺栓规格 螺母规格 螺丝尺寸 螺栓 螺母 螺帽 螺杆 丝杆
    螺纹 内螺纹 外螺纹 螺距 螺纹孔 攻丝 丝锥 板牙 丝锥扳手 自攻螺丝
    自攻钉 木螺丝 膨胀螺栓 膨胀螺丝 膨胀管 膨胀塞 拉爆螺丝 拉铆钉
    铆钉 铆钉枪 铆接 螺丝滑牙 滑丝 滑牙 断螺丝 螺丝生锈 防锈 除锈
    平垫 垫片 平垫片 弹垫 弹簧垫圈 垫圈 防松垫圈 防松螺母 锁紧螺母
    防松胶 螺纹胶 螺纹密封 生料带 螺纹带 润滑油 润滑脂 机械油 黄油枪
    黄油嘴 注油 注油孔 注油嘴 上油 加油润滑 擦拭 除油 清洁剂 除锈剂
    尖嘴钳 钢丝钳 斜口钳 水泵钳 管钳 大力钳 卡簧钳 剥线钳 压线钳
    端子钳 拔钉钳 断线钳 剪线钳 线缆剪 电工钳 钳子 夹钳 固定夹 夹具
    台钳 虎钳 木工夹 弹簧夹 快速夹 夹紧 压紧夹 夹紧器 夹紧螺丝
    锤子 铁锤 橡胶锤 木槌 木锤 小锤 羊角锤 榔头 橡皮锤 钉锤
    撬棍 撬棒 起钉器 拔钉器 凿子 木工凿 錾子 石工凿 冲子 中心冲
    手锯 木工锯 钢锯 细齿锯 粗齿锯 锯条 锯片 圆锯片 锯齿 锯架
    曲线锯 往复锯 圆锯 台锯 电锯 角磨机 砂轮机 砂轮 砂轮片 切割片
    磨片 打磨片 百叶片 打磨头 砂纸 水砂纸 砂布 砂纸架 打磨机 抛光机
    抛光片 抛光盘 抛光膏 打磨 抛光 磨平 磨光 去毛刺 修边 修边机
    木工刨 刨子 电刨 刨刀 刨花 刀片 美工刀 刻刀 裁纸刀 剪刀 铁皮剪
    小刀 工具刀 拆箱刀 安全刀 刮刀 油灰刀 腻子刀 抹刀 铲刀 铲子
    卷尺 钢卷尺 钢尺 直尺 木尺 角尺 三角尺 游标卡尺 数显卡尺 卡尺
    千分尺 测厚仪 水平尺 水平仪 激光测距 测距仪 测电笔 电笔 万用表
    钳形表 电流表 电压表 电阻表 测温枪 温度计 湿度计 刻度 刻度线
    标线 画线 划线 放线 墨斗 弹线 粉线 墨线 记号笔 油漆笔 标记笔
    木梯 人字梯 折叠梯 伸缩梯 铝合金梯 梯子 扶梯 凳梯 脚手架 工作台
    工具柜 工具车 零件盒 分类盒 配件盒 收纳工具 工具收纳 安全手套
    劳保手套 防割手套 防护眼镜 防护面罩 防尘面罩 防尘口罩 防噪耳罩
    """),
    ("home_installation_plumbing_and_electricity", 5500, """
    水暖配件 水暖管件 水管配件 水管安装 水管更换 水路改造 水路维修
    管件 管箍 管卡 管夹 管套 管帽 管堵 堵头 弯头 直通 三通接头
    异径接头 快速接头 活接 活接头 活接弯头 铜接头 铜管 铝管 塑料管
    软管 编织软管 波纹管 金属软管 橡胶软管 进水软管 出水软管 排水软管
    不锈钢管 镀锌管 复合管 冷水管 热水管 供水管 供水阀 供水总阀
    进水阀 排水阀 混水阀 球阀 蝶阀 闸阀 截止阀 减压阀 安全阀
    调节阀 温控阀 恒温阀 放气阀 泄压阀 过滤阀 单向阀 逆止阀 止回阀
    过滤网 水表 水表读数 水表箱 水表更换 管道漏水 管道维修 管道更换
    水管焊接 热熔 热熔器 热熔机 冷凝水管 回水管 回水阀 排气阀
    分水器 集水器 分集水器 地暖 地暖管 地暖维修 地暖清洗 暖气 暖气管
    暖气片 暖气阀 暖气放气 暖气漏水 采暖 供暖 暖气不热 供热报修
    供热管道 供热站 供暖温度 水温 室温 水温调节 恒温花洒 手持花洒
    花洒 花洒头 花洒软管 花洒支架 花洒升降 花洒套装 顶喷 顶喷花洒
    淋浴头 淋浴软管 淋浴龙头 淋浴房 淋浴屏 淋浴隔断 浴缸 面盆 台盆
    洗手盆 洗脸盆 洗漱台 洗手台 浴室柜 镜柜 镜灯 梳妆镜 浴室镜
    台下盆 台上盆 一体盆 陶瓷盆 不锈钢盆 水槽 洗菜盆 双槽 单槽
    龙头出水 起泡器 起泡头 龙头滤网 龙头阀芯 阀芯 密封圈 密封垫
    橡胶圈 防水圈 垫圈 防臭芯 地漏芯 防臭地漏 深水地漏 侧排地漏
    直排地漏 洗衣地漏 地漏下水 下水器 下水配件 下水软管 存水弯
    排污口 排气管 排烟管 排风管 通风管 风管 弯管 挡风板 止逆阀
    油烟止逆 烟道 公共烟道 排气扇 排风扇 换气扇 换气管 通风口
    新风口 送风口 回风口 新风机 新风滤网 风口 风口百叶 百叶窗
    电路改造 电路安装 电线接头 接线端子 接线盒 接线帽 压线帽 线卡
    线槽 线管 波纹线管 穿线管 穿线器 引线器 穿线 穿管 布线 暗线 明线
    暗盒 底盒 开关盒 插座盒 面板盒 接线板 插座面板 开关面板 墙壁开关
    双联开关 单联开关 双控开关 单控开关 三联开关 多联开关 插座开关
    智能面板 调光开关 感应开关 延时开关 声控开关 触摸开关 复位开关
    电源插座 五孔插座 三孔插座 两孔插座 七孔插座 十孔插座 带开关座
    防水插座 地插 地面插座 墙插插座 明装插座 暗装插座 插座底盒
    电源插头 两脚插头 三脚插头 双脚插头 国标插头 旅行插头 转换插头
    转换插座 多用插座 组合插座 插头转换 接地插头 接地线 地线 零线 火线
    接地 接零 接地端子 保护接地 断路器 漏电保护 过载保护 保险丝 熔断器
    电路短路 电路断路 电路断电 电路故障 接触不良 线路松动 线路老化
    家用电器 额定电流 功率负载 电力负荷 用电安全 断电操作 安全用电
    吸顶灯 吊灯 壁灯 筒灯 射灯 轨道灯 轨道射灯 嵌入灯 灯盘 灯座
    灯头 灯口 灯脚 灯泡接口 灯管接口 球泡灯 蜡烛灯 灯泡色温 光照强度
    """),
    ("communication_devices_and_accessories", 5500, """
    通讯设备 通信设备 通话设备 通信配件 通讯配件 通信模块 通讯模块
    信号模块 天线模块 天线接口 天线插头 室内天线 室外天线 外置天线
    内置天线 增益天线 天线增益 天线方向 天线长度 天线安装 天线馈线
    馈线 馈线接头 同轴线 同轴电缆 同轴接口 同轴接头 信号接头 信号线
    语音通话 高清通话 高清语音 语音质量 通话质量 通话清晰 通话音量
    通话声音 通话杂音 通话噪声 通话回声 回声消除 降噪通话 通话录音
    通话转接 呼叫转接 呼叫等待 来电等待 来电转移 无条件转 遇忙转移
    无应答转 呼叫保持 保持通话 暂停通话 通话静音 取消静音 通话免提
    来电显示 来电号码 来电姓名 来电归属 来电铃声 去电号码 去电显示
    未接来电 已接来电 拨出来电 拨出电话 拨入电话 通话时长 通话时间
    通话记录 呼叫记录 拨号键盘 拨号面板 拨号界面 快速拨号 语音拨号
    重拨 自动重拨 一键重拨 最近通话 常用号码 紧急号码 紧急呼叫
    号码归属 号码备注 号码显示 号码隐藏 号码屏蔽 号码拦截 电话骚扰
    骚扰电话 营销电话 推销电话 陌生电话 陌生来电 拦截来电 拦截电话
    电话忙音 电话占线 占线 忙音 无人接听 电话停机 电话欠费 号码停机
    欠费停机 停机保号 号码注销 补办号码 补卡 换卡 换号 保号 携号转网
    号码转网 转网申请 转网码 授权码 话费 话费余额 话费账单 话费充值
    话费缴纳 缴纳话费 话费查询 话费详单 流量账单 流量详单 通话详单
    通话套餐 流量套餐 套餐外 套餐内 超出套餐 套餐到期 套餐续费
    号码套餐 副卡 主卡 主副卡 家庭副卡 共享副卡 亲情号码 亲情号
    亲情套餐 亲情网 家庭套餐 共享套餐 共享流量 共享通话 共享话费
    流量卡 上网卡 物联网卡 电话卡 充值卡 空白卡 卡槽 双卡槽 卡托
    卡针 取卡针 双卡单待 双卡双待 双卡双通 单卡模式 双卡模式
    电子卡 电子号码 设备号码 出厂号码 设备序号 序列号码 识别号码
    电话手柄 电话底座 电话座机 无绳电话 有绳电话 无线电话 免提电话
    子母机 子机 母机 手柄线 弹簧线 电话线 电话接口 电话插头 电话插座
    无线对讲 对讲机 对讲设备 对讲频道 对讲频率 对讲天线 对讲耳机
    对讲电池 对讲充电 对讲底座 手咪 肩咪 耳麦 头戴话筒 领夹话筒
    通讯耳机 通话耳机 通话麦克 麦克风线 麦克风架 话筒架 话筒套
    麦克风套 海绵套 海绵罩 防风套 防风棉 话筒夹 话筒支架 话筒线
    音频插头 音频插孔 耳机插孔 耳机插头 耳麦接口 麦克接口 录音接口
    音频接头 音频转接 音频转线 双头线 单头线 分线 双头插头 单头插头
    音频分线 耳机分线 线控 线控耳机 线控按钮 线控话筒 线控麦克
    信号转换 信号分配 信号传输 信号转接 信号延长 信号接收 信号发送
    转接接口 接口转换 接口转接 转换接口 延长接头 数据插头 数据插孔
    数据信号 视频插头 视频插孔 视频转接 高清接口 高清插头 高清插孔
    视频转线 音视频线 音视频口 音视频头 数据线头 充电线头 充电尾插
    """),
    ("exercise_equipment_movements_and_games", 5500, """
    健身房 健身馆 健身工作 私教 私教课 私人教练 健身教练 运动教练
    体能教练 训练教练 运动指导 健身指导 训练指导 体能指导 动作指导
    训练场 训练馆 训练基地 训练场地 训练区域 力量区 有氧区 拉伸区
    自由区 综合区 休息区 运动区 器械区 器械训练 器械使用 器械重量
    器械调节 器械调试 器械维护 器械保养 重量片 杠铃片 配重片 配重块
    重量调节 配重调节 重量选择 配重选择 握距 握法 握杆 握把 握柄
    把手 把柄 手柄 阻力调节 阻力等级 阻力强度 踏频 踏板 脚踏 脚蹬
    脚踏板 脚踏车 固定单车 卧式单车 立式单车 单车座椅 单车座垫
    座椅高度 座椅调节 靠背调节 靠背角度 坐垫调节 座垫调节 健身座椅
    卧推凳 平板凳 上斜凳 可调凳 可调哑铃 固定哑铃 哑铃架 杠铃架
    深蹲架 力量架 半框架 史密斯架 史密斯机 龙门架 综合训练 拉力架
    配重架 杠铃杆 奥杆 直杆 曲杆 短杆 长杆 训练杆 负重杆 负重背心
    负重腰带 负重腕带 负重脚带 负重沙袋 脚踝沙袋 手腕沙袋 沙袋 沙包
    拳击沙袋 拳击手套 拳击绷带 拳击护具 护齿 护头 护胸 护裆 护胫
    拳靶 手靶 脚靶 腿靶 护腿 护小腿 训练护具 运动护具 护具套装
    运动头带 运动发带 运动手环 运动腰包 运动水壶 运动毛巾 速干衣 速干裤
    速干背心 速干短裤 速干长裤 压缩衣 压缩裤 压缩腿套 弹力护膝
    弹力护腕 弹力护踝 护膝带 护踝带 护腕带 助力带 助力钩 握力带
    握力器 指力器 手指握力 腕力器 臂力器 单杠 双杠 室内单杠 门框单杠
    俯卧撑架 俯卧撑板 健腹轮 腹肌轮 滚轮 滑盘 滑板训练 核心滑盘
    平衡半球 平衡器 波速球 瑞士球 弹力球 重力球 壶铃摆荡 壶铃甩摆
    哑铃抓举 哑铃挺举 杠铃抓举 杠铃挺举 抓举 挺举 高翻 高拉 窄握卧推
    宽握卧推 窄握下拉 宽握下拉 正握下拉 反握下拉 中立握 正握 反握
    对握 坐姿腿举 站姿腿举 坐姿伸腿 俯卧屈腿 坐姿屈腿 哈克深蹲 腿举
    倒蹬 伸腿 屈腿 屈膝 伸膝 屈髋 伸髋 展髋 收髋 背部伸展 反向挺身
    山羊挺身 侧向挺身 上背拉伸 下背拉伸 腰背拉伸 肩颈拉伸 胸部拉伸
    髋部拉伸 小腿拉伸 大腿拉伸 手臂拉伸 腕部拉伸 踝部拉伸 靠墙深蹲
    靠墙静蹲 静蹲 靠墙支撑 坐姿抬腿 仰卧抬腿 仰卧蹬车 空中蹬车
    俄罗斯转 侧卧抬腿 站姿抬腿 站姿侧抬 侧向行走 弹力侧走 单腿站立
    单腿平衡 单腿蹲 半蹲 全蹲 单脚跳 双脚跳 交替跳 跨步跑 跨步走
    交替抬腿 交替抬膝 抱膝 抱膝拉伸 屈膝抱腿 仰卧抱膝 蜘蛛爬行 熊爬
    鸟狗式 死虫式 猫牛式 婴儿式 下犬式 上犬式 战士式 树式 桥式
    山式 坐姿前屈 站姿前屈 坐角式 蝴蝶式 眼镜蛇式 三角式 四点支撑
    侧弯 体侧屈 躯干旋转 胸椎旋转 胸椎伸展 颈部旋转 肩外旋 肩内旋
    地面训练 徒手训练 自重训练 负重训练 循环训练 超级组 递减组 递增组
    正式组 热身组 工作组 力竭组 接近力竭 留有余力 动作变形 训练疲劳
    恢复日 休息日 训练日 练背 练胸 练腿 练肩 练臂 练腹 练臀 全身训练
    """),
    ("public_services_application_and_payments", 6000, """
    办理业务 办理手续 业务办理 业务受理 业务咨询 业务查询 业务登记
    业务申请 业务变更 业务撤销 业务终止 业务办结 办理结果 办理状态
    审批流程 审批结果 审批进度 审批状态 审批意见 审批时间 审批期限
    审批条件 审批材料 审批通过 审批退回 审批不予 许可申请 许可决定
    行政许可 行政审批 行政备案 行政确认 行政裁决 行政复议 行政服务
    公共服务 便民事项 办事事项 政务事项 受理事项 许可事项 备案事项
    事项名称 事项编号 事项编码 事项清单 事项范围 事项说明 事项类别
    办件材料 办件进度 办件状态 办件结果 办件受理 办件领取 受理机构
    受理部门 办理机构 办理部门 审批机构 审批部门 主管部门 归口部门
    办理地址 受理地址 窗口地址 机构地址 主管单位 经办机构 经办部门
    申请日期 申请时间 受理日期 受理时间 受理结果 受理状态 已受理
    未受理 受理中 不予受理 重新受理 受理失败 受理成功 申请撤回
    撤回申请 撤销申请 取消申请 终止申请 申请作废 作废申请 申请注销
    申请更正 申请补正 申请复核 申请查询 申请延长 延期申请 延期审批
    资料更正 资料更新 信息更正 信息补正 更正申请 更正证明 补充证明
    核验结果 核验状态 核验通过 核验失败 核验记录 查验结果 查验记录
    网上申报 在线申报 申报登记 申报资料 申报信息 申报条件 申报时间
    申报期限 申报截止 申报年度 申报月份 申报周期 申报结果 申报状态
    申报成功 申报失败 申报退回 申报修改 申报更正 重新申报 补充申报
    信息采集 数据采集 资料采集 采集表 登记表 登记册 登记信息 登记地址
    登記编号 登记日期 登记时间 登记状态 登记结果 登记成功 登记失败
    登记变更 变更事项 变更内容 变更申请 变更材料 变更手续 变更证明
    注销申请 注销材料 注销手续 注销登记 注销证明 注销结果 撤销登记
    本人签名 申请签名 亲笔签名 手写签名 本人签字 签字确认 签名确认
    签字日期 签字盖章 签名盖章 单位盖章 公司盖章 加盖公章 盖章证明
    公章 印章 名章 私章 法人章 财务章 合同章 专用章 骑缝章 盖章
    办公章 电子章 电子印章 电子公章 印章备案 用章申请 用章审批 用章登记
    收费项目 收费依据 收费金额 收费明细 收费票据 收费凭证 缴费单
    缴费单号 缴费金额 缴费方式 缴费时间 缴费期限 缴费通知 缴费项目
    缴费成功 缴费失败 缴费退款 缴费退还 缴费申请 逾期缴费 补缴费用
    补缴 补缴申请 补缴记录 补缴金额 补缴期限 补缴证明 补交费用
    缴费回执 缴费账单 缴费票据 支付回执 支付票据 支付证明 电子票据
    非税缴费 非税收入 非税票据 财政票据 票据核验 票据查询 票据号码
    票据代码 票据打印 票据补打 票据作废 票据补办 票据更正 发票补打
    发票补开 发票更正 发票作废 红字发票 蓝字发票 冲红 红冲 冲红申请
    发票红冲 税额 含税 未税 不含税 税前 税后 含税价 未税价 税率
    个人所得 所得税 增值税 营业税 契税 印花税 房产税 土地税 车船税
    """),
    ("time_duration_and_daily_progress", 6000, """
    时间段 时段 时间点 时间差 时间轴 时间线 时间表 时间窗 时间窗口
    日程单 日程计划 日程提醒 行程提醒 时间提醒 待办提醒 定时提醒
    定时通知 到点提醒 准点提醒 按时提醒 提前提醒 延迟提醒 延后提醒
    最近几天 近几天 这两天 前两天 后两天 头两天 前一天 后一天 同一天
    第二天 第三天 头一天 头几天 隔一天 隔两天 过几天 过两天 过一天
    今天早上 今天中午 今天夜里 今晚 明早 明晚 明午 后天 后日 次日
    当晚 当夜 次晚 次夜 次晨 当晨 今晨 昨夜 昨晚 前晚 前夜 前日
    白昼 白天 夜间 夜晚 夜里 夜半 深夜 午夜 子夜 半夜 凌晨 黎明 拂晓
    清晨 早晨 晨间 午前 午间 午后 晚间 傍晚 黄昏 日暮 薄暮 入夜 夜深
    早些时候 晚些时候 前些时候 近些时候 早一点 晚一点 早点儿 晚点儿
    再早点 再晚点 早了一点 晚了一点 有点早 有点晚 有些早 有些晚
    稍微早些 稍微晚些 稍早一点 稍晚一点 提前一点 推迟一点 稍作等候
    等候片刻 等待片刻 等待一会 稍等一会 稍等会儿 等多一会 再等一会
    再等等 多等一会 等会再来 等会再去 等会再看 等会再办 一会再来
    一会再去 一会再看 一会再办 晚点再来 晚点再去 晚点再看 晚点再办
    隔天 隔日 隔周 隔月 隔年 逐日 逐周 逐月 逐季 逐年 按日 按周
    按月 按季 按年 每日 每周 每月 每季 每年 每隔 每隔一天 每隔两天
    每隔一周 每隔两周 每隔一月 每隔两月 每隔一年 每隔半年 半日 半天
    半个月 一个月 一两天 两三天 三四天 四五天 五六天 两三周 一两周
    三四周 一两月 两三月 几个月 几年 几十年 好几年 多半年 大半年
    小半年 小半天 大半天 小半个月 大半个月 一小会儿 好一阵子 一阵子
    一阵儿 一阵 一阵一阵 一时半会 一时半刻 转眼间 转瞬间 一眨眼 一转眼
    不一会儿 不一会 没一会 没一会儿 没过多久 没过几天 没隔多久 过不了久
    不久之后 很快之后 随后不久 后来不久 过后不久 片刻之后 转瞬之间
    马上就好 很快就好 一会就好 快好了 快完成了 快结束了 刚开始 刚刚开始
    已经开始 已经结束 尚未开始 尚未结束 暂未开始 暂未结束 还没开始
    还没结束 正要开始 正要结束 即将开始 即将结束 开始之前 结束之后
    开始以后 结束以后 完成以后 完成之后 未完成前 做完以后 做完之后
    做好以后 做好之后 办完以后 办完之后 忙完以后 忙完之后 结束时间
    完成时间 开始时间 准备时间 处理时间 等待时间 响应时间 休息时间
    延后 延缓 延迟 顺延 提早 提前 推后 推迟 提速 加快 放缓 减慢
    拖后 拖延 拖慢 耽误 耽搁 拖得太久 等得太久 等了很久 花了很久
    用了很久 拖了很久 拖了几天 等了几天 等了一天 等了半天 等了一会
    来得及了 来不及了 赶不上了 赶得上了 来早了 来晚了 到早了 到晚了
    晚了一步 早了一步 快了一步 慢了一步 慢半拍 快半拍 赶上了 错过了
    准点到达 按时到达 按时到场 到达时间 离开时间 返回时间 回来时间
    空闲时间 闲暇时间 业余时间 休闲时间 课余时间 业余 空余 闲暇 空闲
    """),
]

OVERRIDES = {
    "卡尺": "kachi", "游标卡尺": "youbiaokachi", "数显卡尺": "shuxiankachi",
    "钳形表": "qianxingbiao", "起钉器": "qidingqi", "拔钉器": "badingqi",
    "地钉": "diding", "剥线钳": "boxianqian", "刨子": "baozi", "木工刨": "mugongbao",
    "电刨": "dianbao", "刨刀": "baodao", "刨花": "baohua", "凿子": "zaozi",
    "錾子": "zanzi", "角磨机": "jiaomoji", "六角扳手": "liujiaobanshou",
    "拧紧": "ningjin", "长杆": "changgan", "长线": "changxian", "加长批头": "jiachangpitou",
    "弹垫": "tandian", "弹簧垫圈": "tanhuangdianquan", "弹簧夹": "tanhuangjia",
    "弹簧线": "tanhuangxian", "弹力护膝": "tanlihuxi", "弹力护腕": "tanlihuwan",
    "弹力护踝": "tanlihuhuai", "弹力侧走": "tanlicezou", "弹线": "tanxian",
    "冲子": "chongzi", "中心冲": "zhongxinchong", "冲击钻": "chongjizuan",
    "朝前": "chaoqian", "卡簧钳": "kahuangqian", "卡槽": "kacao", "卡托": "katuo",
    "卡针": "kazhen", "取卡针": "qukazhen", "上网卡": "shangwangka",
    "台钳": "taiqian", "台下盆": "taixiapen", "台上盆": "taishangpen",
    "灯杆": "denggan", "延长杆": "yanchanggan", "开孔钻": "kaikongzuan",
    "阶梯钻": "jietizuan", "扩孔钻": "kuokongzuan", "木工钻": "mugongzuan",
    "金属钻": "jinshuzuan", "水泥钻": "shuinizuan", "瓷砖钻": "cizhuanzuan",
    "玻璃钻": "bolizuan", "麻花钻": "mahuazuan", "钻头": "zuantou",
    "测厚仪": "cehouyi", "测电笔": "cedianbi", "穿管": "chuanguan",
    "管卡": "guanka", "管夹": "guanjia", "铜接头": "tongjietou",
    "热熔器": "rerongqi", "热熔机": "rerongji", "分集水器": "fenjishuiqi",
    "地暖": "dinuan", "地暖管": "dinuanguan", "地暖清洗": "dinuanqingxi",
    "地暖维修": "dinuanweixiu", "供水管": "gongshuiguan",
    "供水阀": "gongshuifa", "供水总阀": "gongshuizongfa", "供热报修": "gongrebaoxiu",
    "供热管道": "gongreguandao", "供热站": "gongrezhan", "供暖温度": "gongnuanwendu",
    "放气阀": "fangqifa", "阀芯": "faxin", "采暖": "cainuan", "调节阀": "tiaojiefa",
    "风口百叶": "fengkoubaiye", "百叶窗": "baiyechuang", "给排水": "jipaishui",
    "恒温花洒": "hengwenhuasa", "手持花洒": "shouchihuasa", "起泡头": "qipaotou",
    "滤网": "lvwang", "过滤阀": "guolvfa", "过滤网": "guolvwang", "新风滤网": "xinfenglvwang",
    "龙头滤网": "longtoulvwang", "电线接头": "dianxianjietou", "接线盒": "jiexianhe",
    "接线端子": "jiexianduanzi", "穿线": "chuanxian", "穿线器": "chuanxianqi",
    "墙壁开关": "qiangbikaiguan", "多用插座": "duoyongchazuo", "负荷": "fuhe",
    "电力负荷": "dianlifuhe", "额定电流": "edingdianliu", "插座调试": "chazuotiaoshi",
    "灯泡色温": "dengpaosewen", "光照强度": "guangzhaoqiangdu",
    "重拨": "chongbo", "自动重拨": "zidongchongbo", "一键重拨": "yijianchongbo",
    "亲情网": "qinqingwang", "亲情号码": "qinqinghaoma", "亲情号": "qinqinghao",
    "亲情套餐": "qinqingtaocan", "号码停机": "haomatingji", "号码归属": "haomaguishu",
    "序列号码": "xuliehaoma", "话费详单": "huafeixiangdan", "流量详单": "liuliangxiangdan",
    "通话详单": "tonghuaxiangdan", "双卡单待": "shuangkadandai", "单卡模式": "dankamoshi",
    "麦克风线": "maikefengxian", "话筒夹": "huatongjia", "噪声": "zaosheng",
    "线控": "xiankong", "耳麦": "ermai", "识别号码": "shibiehaoma",
    "无应答转": "wuyingdazhuan", "应答": "yingda", "应答转移": "yingdazhuanyi",
    "通话转接": "tonghuazhuanjie", "呼叫转接": "hujiaozhuanjie", "接口转换": "jiekouzhuanhuan",
    "信号转换": "xinhaozhuanhuan", "信号转接": "xinhaozhuanjie", "数据插孔": "shujuchakong",
    "坐姿腿举": "zuozituiju", "俯卧屈腿": "fuwoqutui", "坐姿屈腿": "zuoziqutui",
    "屈腿": "qutui", "屈膝": "quxi", "屈髋": "qukuan", "站姿前屈": "zhanziqianqu",
    "坐姿前屈": "zuoziqianqu", "坐角式": "zuojiaoshi", "眼镜蛇式": "yanjingsheshi",
    "体侧屈": "ticequ", "肩外旋": "jianwaixuan", "肩内旋": "jianneixuan",
    "负重训练": "fuzhongxunlian", "自重训练": "zizhongxunlian", "负重杆": "fuzhonggan",
    "负重背心": "fuzhongbeixin", "负重腰带": "fuzhongyaodai", "负重腕带": "fuzhongwandai",
    "负重脚带": "fuzhongjiaodai", "负重沙袋": "fuzhongshadai", "重量片": "zhongliangpian",
    "重量调节": "zhongliangtiaojie", "重量选择": "zhongliangxuanze", "阻力调节": "zulitiaojie",
    "配重": "peizhong", "配重片": "peizhongpian", "配重块": "peizhongkuai",
    "配重调节": "peizhongtiaojie", "配重选择": "peizhongxuanze", "配重架": "peizhongjia",
    "单腿蹲": "dantuidun", "单脚跳": "danjiaotiao",
    "四点支撑": "sidianzhicheng", "鸟狗式": "niaogoushi", "死虫式": "sichongshi",
    "熊爬": "xiongpa", "交替抬膝": "jiaotitaixi", "屈膝抱腿": "quxibaotui",
    "仰卧抬腿": "yangwotaitui", "仰卧蹬车": "yangwodengche", "仰卧抱膝": "yangwobaoxi",
    "手腕沙袋": "shouwanshadai", "指力器": "zhiliqi", "脚踝沙袋": "jiaohuaishadai",
    "护裆": "hudang", "护胫": "hujing", "哈克深蹲": "hakeshendun", "倒蹬": "daodeng",
    "卧推凳": "wotuideng", "直杆": "zhigan", "曲杆": "qugan", "短杆": "duangan",
    "训练杆": "xunliangan", "正握": "zhengwo", "反握": "fanwo", "宽握下拉": "kuanwoxiala",
    "正握下拉": "zhengwoxiala", "反握下拉": "fanwoxiala", "肩颈拉伸": "jianjinglashen",
    "颈部旋转": "jingbuxuanzhuan", "助力带": "zhulidai", "助力钩": "zhuligou",
    "动作变形": "dongzuobianxing", "体能教练": "tinengjiaolian", "私教课": "sijiaoke",
    "用章申请": "yongzhangshenqing", "印章备案": "yinzhangbeian", "亲笔签名": "qinbiqianming",
    "重新受理": "chongxinshouli", "重新申报": "chongxinshenbao", "资料更正": "ziliaogengzheng",
    "信息更正": "xinxigengzheng", "更正申请": "gengzhengshenqing", "更正证明": "gengzhengzhengming",
    "申报更正": "shenbaogengzheng", "票据更正": "piaojugengzheng", "发票更正": "fapiaogengzheng",
    "征收": "zhengshou", "非税缴费": "feishuijiaofei", "缴费账单": "jiaofeizhangdan",
    "缴费通知": "jiaofeitongzhi", "补缴": "bujiao", "补缴申请": "bujiaoshenqing",
    "补缴记录": "bujiaojilu", "补缴金额": "bujiaojine", "补缴期限": "bujiaoqixian",
    "补缴证明": "bujiaozhengming", "单位盖章": "danweigaizhang", "公司盖章": "gongsigaizhang",
    "骑缝章": "qifengzhang", "加盖公章": "jiagaigongzhang", "印花税": "yinhuashui",
    "契税": "qishui", "冲红": "chonghong", "冲红申请": "chonghongshenqing", "红冲": "hongchong",
    "发票红冲": "fapiaohongchong", "准点到达": "zhundiandaoda",
    "来得及了": "laidejile", "赶得上了": "gandeshangle", "等得太久": "dengdetaijiu",
    "拖得太久": "tuodetaijiu", "空闲时间": "kongxianshijian", "空余": "kongyu",
    "闲暇时间": "xianxiashijian", "近些时候": "jinxieshihou", "薄暮": "bomu",
    "长久": "changjiu", "明朝": "mingzhao", "一会再来": "yihuizailai",
    "一会再去": "yihuizaiqu", "一会再看": "yihuizaikan", "一会再办": "yihuizaiban",
    "刚刚开始": "ganggangkaishi", "一时半会": "yishibanhui", "稍等会儿": "shaodenghuier",
    "还没开始": "haimeikaishi", "还没结束": "haimeijieshu", "多少": "duoshao",
    "处理时间": "chulishijian", "响应时间": "xiangyingshijian", "时间差": "shijiancha",
    "垫圈": "dianquan", "防松垫圈": "fangsongdianquan", "可调凳": "ketiaodeng",
    "可调哑铃": "ketiaoyaling",
}

EXCLUDED = {
    "电动螺丝", "磁性螺丝", "螺纹密封", "收纳工具", "花洒升降", "龙头出水",
    "油烟止逆", "带开关座", "墙插插座", "功率负载", "来电归属", "无条件转",
    "无应答转", "拨出来电", "通话麦克", "麦克接口", "线控麦克", "视频转线",
    "音频转线", "音视频口", "音视频头", "健身工作", "俄罗斯转", "站姿侧抬",
    "审批不予", "申请签名", "登记编号", "登記编号", "办公章", "个人所得",
    "过不了久", "很快之后", "未完成前", "空白卡", "号码套餐", "加油润滑",
    "明午", "次晚", "次夜", "当晨", "拖后", "到早了", "到晚了", "半框架", "站姿腿举",
}

POLYPHONES = "行长重调假还得藏薄处校系载便单转觉乐宿量数供应间差弹传解着难干圈缝会空省发划钻泊铺屈强夹纶粘兴好拗露辟落翘折角率卷鲜降参盛秘斗闷蒙漂踏纤蚕蕃质颈杆壳舍脏脯咽咳骨累识刨剥拓卡扇扁冠熨择给荷腊屏尺混"

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> None:
    base = ROOT / "resources/dictionary"
    baselines = [base / name for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")]
    baselines += [OUT / f"lexicon_batch_{n:03d}.tsv" for n in range(1, 7)]
    known_keys, known_words = set(), set()
    for path in baselines:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                key, word, _ = line.split("\t")
                known_keys.add((key, word)); known_words.add(word)
    benchmarks = sorted((ROOT / "tests/data").glob("typing_*.tsv"))
    benchmark_answers = set()
    for path in benchmarks:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                benchmark_answers.add(line.split("\t")[1])
    selected, seen, excluded_alternate = [], set(), []
    draft_count = 0
    for category, frequency, text in SECTIONS:
        for word in text.split():
            draft_count += 1
            if word in seen or word in EXCLUDED or not re.fullmatch(r"[\u4e00-\u9fff]{2,4}", word):
                continue
            seen.add(word)
            if word in benchmark_answers:
                raise ValueError(f"Benchmark answer in draft: {word}")
            key = OVERRIDES.get(word) or "".join(lazy_pinyin(word, style=Style.NORMAL, strict=True))
            if not re.fullmatch(r"[a-z]+", key):
                raise ValueError(f"Invalid pronunciation: {word} {key}")
            if (key, word) in known_keys:
                continue
            if word in known_words:
                excluded_alternate.append({"word": word, "proposed_pinyin": key}); continue
            selected.append((category, key, word, frequency))
    selected.sort(key=lambda r: (r[0], r[1], r[2]))
    assert len(selected) == len({(key, word) for category, key, word, frequency in selected})
    for category, key, word, frequency in selected:
        positions = {0}
        for char in word:
            choices = pinyin(char, style=Style.NORMAL, heteronym=True, strict=True)[0]
            positions = {i + len(reading) for i in positions for reading in choices if key.startswith(reading, i)}
        if len(key) not in positions:
            raise ValueError(f"Impossible character reading: {key} {word}")
        assert (key, word) not in known_keys and word not in known_words
    lines, previous = [], None
    for category, key, word, frequency in selected:
        if previous != category:
            lines.append(f"# category: {category}"); previous = category
        lines.append(f"{key}\t{word}\t{frequency}")
    output = OUT / "lexicon_batch_007.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    selected_words = {r[2] for r in selected}
    metadata = {
        "batch": "lexicon_batch_007", "authoring_source": "first_party_authored",
        "external_lexicon_import": False, "pronunciation_tool": "pypinyin authoring-only; explicit reviewed compound overrides",
        "authored_candidates": draft_count, "new_entries": len(selected), "unique_words": len(selected_words),
        "sha256": sha(output), "category_counts": dict(sorted(Counter(r[0] for r in selected).items())),
        "length_distribution": dict(sorted(Counter(str(len(r[2])) for r in selected).items())),
        "frequency_range": [min(r[3] for r in selected), max(r[3] for r in selected)],
        "dedup_policy": "Exclude existing key/display pair and existing display words with alternate generated readings",
        "dedup_baseline_sha256": {p.name: sha(p) for p in baselines},
        "benchmark_sha256": {p.name: sha(p) for p in benchmarks}, "benchmark_full_answers_added": 0,
        "alternate_pronunciation_candidates_excluded": excluded_alternate,
        "manual_pronunciation_overrides": {w: k for w, k in OVERRIDES.items() if w in selected_words},
        "manual_review_corrections": {"防松垫圈": "fangsongdianquan", "可调凳": "ketiaodeng", "可调哑铃": "ketiaoyaling"},
        "u_v_policy": "Preserve standard pypinyin ASCII v for umlaut-u readings (for example lvxing and lvwang); no broad u/v alias generation.",
        "validation": {"unique_key_display_pairs": len(selected), "format_valid": True,
            "all_outputs_two_to_four_han": True, "per_character_possible_readings_valid": True,
            "canonical_or_previous_batch_collisions": 0, "benchmark_full_answers_added": 0,
            "polyphone_reviewed_display_words": sum(any(c in w for c in POLYPHONES) for w in selected_words)},
        "canonical_dictionaries_modified": False,
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: metadata[k] for k in ("new_entries", "authored_candidates", "sha256", "category_counts")}, ensure_ascii=False))

if __name__ == "__main__":
    main()
