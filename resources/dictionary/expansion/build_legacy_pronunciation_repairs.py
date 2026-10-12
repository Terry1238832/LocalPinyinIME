#!/usr/bin/env python3
"""Produce a reviewed repair plan; never modify the runtime dictionaries."""
from __future__ import annotations
import hashlib
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
REPORT = ROOT / ".build/evaluation/pronunciation-impossible.json"

# The entire finite family was read and reviewed, including the suffix keys.
# Only the explicitly listed useful combinations are corrected. These lists
# do not add new combinations or derive pronunciations with an external tool.
REVIEWED_ACTIONS = {
    "吃饭": "chifan", "吃晚饭": "chiwanfan", "吃早餐": "chizaocan",
    "吃午餐": "chiwucan", "喝水": "heshui", "喝点水": "hedianshui",
    "喝牛奶": "heniunai", "喝咖啡": "hekafei", "回家": "huijia",
    "出门": "chumen", "等车": "dengche", "打车": "dache",
    "坐地铁": "zuoditie", "坐公交": "zuogongjiao", "上课": "shangke",
    "下课": "xiake", "学习": "xuexi", "写作业": "xiezuoye",
    "提交作业": "tijiaozuoye", "检查作业": "jianchazuoye", "看书": "kanshu",
    "做笔记": "zuobiji", "复习课程": "fuxikecheng", "准备考试": "zhunbeikaoshi",
    "看资料": "kanziliao", "找资料": "zhaoziliao", "写报告": "xiebaogao",
    "填写表格": "tianxiebiaoge", "看电影": "kandianying", "听音乐": "tingyinyue",
    "发消息": "faxiaoxi", "回复消息": "huifuxiaoxi", "查看消息": "chakanxiaoxi",
    "打电话": "dadianhua", "打开文件": "dakaiwenjian", "保存文件": "baocunwenjian",
    "删除文件": "shanchuwenjian", "整理文件": "zhengliwenjian", "分享文件": "fenxiangwenjian",
    "备份文件": "beifenwenjian", "下载软件": "xiazairuanjian", "更新系统": "gengxinxitong",
    "重启系统": "chongqixitong", "连接网络": "lianjiewangluo", "检查网络": "jianchawangluo",
    "设置密码": "shezhimima", "打开设置": "dakaishezhi", "更改设置": "genggaishezhi",
    "打开电脑": "dakaidiannao", "关闭电脑": "guanbidiannao", "浏览网页": "liulanwangye",
    "搜索内容": "sousuoneirong", "输入中文": "shuruzhongwen", "切换英文": "qiehuanyingwen",
    "切换中文": "qiehuanzhongwen", "选择候选": "xuanzehouxuan", "输入拼音": "shurupinyin",
    "选择汉字": "xuanzehanzi", "解决问题": "jiejuewenti", "处理问题": "chuliwenti",
}
BAD_COMPLETION_SUFFIXES = {"到了", "快到了"}

# Decisions outside the two finite phrase families are explicit and readable.
# Values are literal syllables, so review does not depend on default polyphony.
REPLACEMENTS = {
    "星期天": ("xing qi tian", "旧键把期和天的拼音顺序颠倒；按星期天的实际字序修正。"),
    "不确定": ("bu que ding", "确定的确应为que；旧键ke与显示词不符。"),
    "复杂": ("fu za", "杂读za，旧键zha错误。"),
    "买东西": ("mai dong xi", "买读mai，旧键mao错误。"),
    "蛮好的": ("man hao de", "蛮读man，旧键mang错误。"),
    "方不方便": ("fang bu fang bian", "旧键漏掉方便中的方；按方不方便四字补齐。"),
    "需不需要": ("xu bu xu yao", "旧键漏掉需要中的需；按需不需要四字补齐。"),
    "怎么可能": ("zen me ke neng", "可读ke，旧键ka错误。"),
    "认真的吗": ("ren zhen de ma", "旧键漏掉的；认真的吗包含de。"),
    "好活": ("hao huo", "活读huo，旧键hu不完整。"),
    "大可不必": ("da ke bu bi", "可读ke，旧键ka错误。"),
    "先这样": ("xian zhe yang", "这读zhe；旧键zhi错误。"),
    "先这样吧": ("xian zhe yang ba", "这读zhe；旧键zhi错误。"),
    "换句话说": ("huan ju hua shuo", "显示词没有一；旧键对应换一句话说，多出的yi应移除。"),
    "注释": ("zhu shi", "释读shi；旧键zhujie是注解的拼音，与注释显示词不符。"),
    "详细日志": ("xiang xi ri zhi", "细读xi；旧键xiangqing对应详情，与详细不符。"),
    "词库里没有": ("ci ku li mei you", "显示词没有却；旧键多出了que。"),
    "整句很难": ("zheng ju hen nan", "很难应为hen nan；旧键hebunan与字序和字音均不符。"),
    "四舍五入": ("si she wu ru", "旧键漏掉舍的she；按四舍五入四字补齐。"),
    "牵引绳": ("qian yin sheng", "引读yin，旧键ying错误。"),
    "梳毛": ("shu mao", "梳读shu，旧键shun错误。"),
    "润发乳": ("run fa ru", "乳读ru；旧键lu与显示词不符。"),
    "湿纸巾": ("shi zhi jin", "纸读zhi；旧键shi错误。"),
    "洗脸盆": ("xi lian pen", "脸读lian；旧键ximianpen对应洗面盆。"),
    "泡澡": ("pao zao", "泡在泡澡中读pao；旧键pai错误。"),
    "闭气": ("bi qi", "气读qi；旧键bixi与闭气不符。"),
    "冠军": ("guan jun", "军读jun；旧键ji缺失尾部字音。"),
    "引体向上": ("yin ti xiang shang", "旧键漏掉体的ti；保留固定运动名称。"),
    "核心训练": ("he xin xun lian", "旧键漏掉心的xin；核心训练是常用运动词。"),
    "起球": ("qi qiu", "旧键把起和球的拼音颠倒；起球是织物表面结小球。"),
    "面料": ("mian liao", "料读liao；旧键diao错误。"),
    "速写本": ("su xie ben", "速写应为su xie；旧键sucao与显示词不符。"),
    "笔袋": ("bi dai", "笔袋为bi dai；旧键多出shi。"),
    "候车区": ("hou che qu", "车读che；旧键ding与显示词不符。"),
    "开题会": ("kai ti hui", "题读ti；旧键xiang与开题会不符。"),
    "加热": ("jia re", "加读jia；旧键jie错误。"),
    "比热": ("bi re", "热读re；旧键she错误。"),
    "拉力": ("la li", "力读li；旧键yi错误。"),
    "初速度": ("chu su du", "显示词没有始；旧键多出了shi。"),
    "形变量": ("xing bian liang", "形变量是形变的量；显示词没有式，移除旧键shi。"),
    "前奏": ("qian zou", "显示词没有引；旧键多出yin。"),
    "木偶剧": ("mu ou ju", "偶读ou、剧读ju；旧键mujuchu与名称不符。"),
    "尼日利亚": ("ni ri li ya", "按中文国名尼日利亚拼音；旧键多出ge，不能作为这四个汉字的拼音。"),
    "避难场所": ("bi nan chang suo", "难为nan、所为suo；旧键min和su错误。"),
    "市政建设": ("shi zheng jian she", "政读zheng；旧键hua错误。"),
    "扁桃体炎": ("bian tao ti yan", "旧键漏掉扁的bian；完整病名按四字补齐。"),
    "近视": ("jin shi", "视读shi；旧键yan对应另一字音。"),
    "签字笔": ("qian zi bi", "字读zi；旧键qianmingbi对应签名笔。"),
    "记号笔": ("ji hao bi", "记读ji；旧键jin错误。"),
    "图钉": ("tu ding", "钉在名词图钉中读ding；旧键zhen错误。"),
    "打孔器": ("da kong qi", "显示词没有笔；旧键多出bi。"),
    "尖嘴钳": ("jian zui qian", "嘴读zui；旧键zuo错误。"),
    "剥线钳": ("bo xian qian", "工具名剥线钳按bo xian qian；旧键ya与线不符。"),
    "管接头": ("guan jie tou", "接读jie；旧键jian错误。"),
    "充电电池": ("chong dian dian chi", "显示词含两个电；旧键漏掉一个dian。"),
    "数位板": ("shu wei ban", "数位板为shu wei ban；旧键shuxieban对应书写板。"),
    "后视镜": ("hou shi jing", "视读shi；旧键pai错误。"),
    "安全气囊": ("an quan qi nang", "囊读nang；旧键lang错误。"),
    "排气管": ("pai qi guan", "旧键漏掉气的qi。"),
    "补漆": ("bu qi", "漆读qi；旧键xi错误。"),
    "玩具手鼓": ("wan ju shou gu", "鼓读gu；旧键gun错误。"),
    "女婿": ("nv xu", "女婿为nv xu；显示词没有儿，移除旧键er。"),
    "婚姻关系": ("hun yin guan xi", "关读guan；旧键guang错误。"),
    "传票": ("chuan piao", "票读piao；旧键tiao错误。"),
    "调取": ("diao qu", "调取是调出取得，调读diao；旧键dia缺字母o。"),
    "在逃": ("zai tao", "在读zai；旧键zao错误。"),
    "工匠": ("gong jiang", "工读gong；旧键shijiang对应石匠。"),
    "波斯帝国": ("bo si di guo", "中文名称是波斯；旧键多出li。"),
    "印加文明": ("yin jia wen ming", "印读yin；旧键ying错误。"),
    "阿兹特克文明": ("a zi te ke wen ming", "兹读zi；旧键si错误。"),
    "民俗节": ("min su jie", "节读jie；旧键ji错误。"),
    "军靴": ("jun xue", "靴读xue；旧键xie错误。"),
    "日立": ("ri li", "名称日立为ri li；旧键rixin对应另一显示词。"),
    "夏普": ("xia pu", "品牌名称夏普为xia pu；旧键多出li。"),
    "沃尔沃": ("wo er wo", "中文名称中保留尔的er；旧键wowo没有合法儿化r尾。"),
    "雀巢": ("que chao", "巢读chao；旧键shi错误。"),
    "吉百利": ("ji bai li", "品牌名吉百利按三字拼音；旧键jiabao与显示词不符。"),
    "霍尼韦尔": ("huo ni wei er", "旧键漏掉韦的wei；保留四字品牌名称。"),
    "牧场": ("mu chang", "显示词没有羊；旧键muyangchang对应牧羊场。"),
    "四则运算": ("si ze yun suan", "旧键漏掉则的ze。"),
    "线段": ("xian duan", "段读duan；旧键shuan错误。"),
    "四边形": ("si bian xing", "边读bian；旧键sijiaoxing对应四角形。"),
    "弹性势能": ("tan xing shi neng", "弹性读tan xing、势读shi；旧键漏掉xing shi。"),
    "突变": ("tu bian", "突读tu；旧键tuo错误。"),
    "捕食": ("bu shi", "食读shi；旧键xi错误。"),
    "日晷": ("ri gui", "晷读gui；旧键kui错误。"),
    "默读": ("mo du", "默读的默读mo；旧键mu错误。"),
    "速滑": ("su hua", "显示词速滑没有冰；旧键多出bing。"),
    "质押": ("zhi ya", "押读ya；旧键yan错误。"),
    "紧急备用金": ("jin ji bei yong jin", "旧键漏掉备的bei；保留固定财务名词。"),
    "财务状况": ("cai wu zhuang kuang", "务读wu；旧键caifu对应财富。"),
    "年度目标": ("nian du mu biao", "度的du后旧键多出一个u。"),
    "模具": ("mu ju", "大陆普通话模具读mu ju；具为ju，旧键mumo不符合任何正常读法。"),
    "插秧": ("cha yang", "秧读yang；旧键cao错误。"),
    "虾塘": ("xia tang", "塘读tang；旧键tan错误。"),
    "结冰": ("jie bing", "冰读bing；旧键jiedong对应结冻。"),
    "资源节约": ("zi yuan jie yue", "节读jie；旧键jian错误。"),
    "毛坯房": ("mao pi fang", "坯读pi；旧键pei错误。"),
    "押一付一": ("ya yi fu yi", "按押一付一实际字序；旧键yifuyayi次序颠倒。"),
    "船袜": ("chuan wa", "显示词没有管；旧键多出guan。"),
    "鸭舌帽": ("ya she mao", "舌读she；旧键shi错误。"),
    "须后水": ("xu hou shui", "按须后水字序；旧键huxushui对应胡须水。"),
    "洗涤": ("xi di", "涤读di；旧键die错误。"),
    "小火": ("xiao huo", "小读xiao；旧键xia缺字母o。"),
    "清理台面": ("qing li tai mian", "台读tai；旧键tao错误。"),
    "异味": ("yi wei", "味读wei；旧键yichou对应异臭。"),
    "隔断": ("ge duan", "断读duan；旧键bian错误。"),
    "水电": ("shui dian", "水读shui；旧键shua错误。"),
    "标记未读": ("biao ji wei du", "记读ji；旧键jing错误。"),
    "邮政编码": ("you zheng bian ma", "旧键漏掉政的zheng；完整显示词需完整拼音。"),
    "邮戳": ("you chuo", "戳读chuo；旧键cuo错误。"),
    "真真切切": ("zhen zhen qie qie", "第二个真读zhen；旧键zheng错误。"),
    "有理有据": ("you li you ju", "第二个有读you；旧键yi错误。"),
    "事在人为": ("shi zai ren wei", "按事在人为实际字序；旧键ren zai颠倒。"),
    "尽心尽力": ("jin xin jin li", "第二个尽读jin；旧键ji错误。"),
    "前因后果": ("qian yin hou guo", "后读hou；旧键huo错误。"),
    "玄关": ("xuan guan", "玄读xuan；旧键xuang错误。"),
    "玄关柜": ("xuan guan gui", "玄读xuan；旧键xuang错误。"),
    "按电梯按钮": ("an dian ti an niu", "按钮按an niu；旧键漏掉第二个按拼音末尾n。"),
    "电梯上来了": ("dian ti shang lai le", "旧键漏掉来的lai。"),
    "电梯下来了": ("dian ti xia lai le", "旧键漏掉来的lai。"),
    "下楼": ("xia lou", "下读xia；旧键xiaolou对应小楼。"),
    "无精打采": ("wu jing da cai", "采读cai；旧键ca缺字母i。"),
    "上下楼": ("shang xia lou", "下读xia；旧键xiaolou中的o不属于上下楼。"),
}

KEEP = {
    "嗯": "en为通用语气词输入键；逐字读音工具只枚举n/ng等音，属于误报，保留。",
    "嗯哼": "enh eng连写为enheng是常见语气词输入键，保留口语别名。",
    "嗯嗯": "enen是常用重复语气词输入键，保留口语别名。",
    "哪儿": "nar是合法儿化拼音压缩写法，保留；不能按na+er机械判错。",
    "这儿": "zher是合法儿化拼音压缩写法，保留；不能按zhe+er机械判错。",
    "那儿": "nar是合法儿化拼音压缩写法，保留；语义与哪儿不同但无声调拼音键可以相同。",
    "遛弯儿": "liuwanr是合法儿化拼音写法，保留wanr。",
    "优盘": "upan是U盘/优盘通用字母读法输入别名，保留；常规you读音不必替换该别名。",
    "摩尔": "mol是物质的量单位摩尔的标准国际符号；保留单位符号输入别名。常规拼音为moer。",
    "电视盒": "dianshi+英文box是可解释的既有混拼输入别名，保留兼容；常规拼音dianshihe另列补键建议。",
    "味醂": "味醂的醂在日式料理酒名称中读lin，weilin正确；工具只枚举单字lan造成误报。教育云字典明确收录lin读音。",
}

REMOVE = {
    "浏览夹": "疑似浏览器/文件夹等词的误合成，不是明确常用软件词或物品词；删除不明草稿词，不臆造读音。",
    "儿童图": "疑似儿童图书/儿童画等词的截断残片，缺少明确独立常用词义；删除不明草稿词。",
}


def main():
    rows = json.loads(REPORT.read_text(encoding="utf-8-sig"))
    output = []
    for source in rows:
        word, old_key = source["word"], source["key"]
        row = {"file": source["file"], "source_line": source["line"], "old_key": old_key, "word": word}
        family = next((f for f in ("可不可以", "需不需要") if word.startswith(f) and len(word) > len(f)), None)
        if family:
            suffix = word[len(family):]
            if suffix in BAD_COMPLETION_SUFFIXES:
                row.update(remove=True, action="remove", reason=f"{family}表示许可/必要性，不能自然接完成状态‘{suffix}’；这是机械模板产生的不通顺残片，删除。")
            else:
                assert suffix in REVIEWED_ACTIONS, (word, "unreviewed suffix")
                old_prefix = "keyibukeyi" if family == "可不可以" else "xubuyao"
                new_prefix = "kebukeyi" if family == "可不可以" else "xubuxuyao"
                assert old_key == old_prefix + REVIEWED_ACTIONS[suffix], (word, old_key)
                row.update(new_key=new_prefix + REVIEWED_ACTIONS[suffix], action="replace",
                           reason=("可不可以为ke bu ke yi；旧键keyibukeyi多出第一个可后的yi。" if family == "可不可以" else "需不需要为xu bu xu yao；旧键xubuyao漏掉需要中的xu。") + f"尾段‘{suffix}’及其拼音已逐条检查，语义通顺，保留并修正前缀。")
        elif word in REPLACEMENTS:
            syllables, reason = REPLACEMENTS[word]
            row.update(new_key="".join(syllables.split()), action="replace", reviewed_syllables=syllables, reason=reason)
        elif word in KEEP:
            row.update(new_key=old_key, action="keep", reason=KEEP[word])
        elif word in REMOVE:
            row.update(remove=True, action="remove", reason=REMOVE[word])
        else:
            raise ValueError(f"Unreviewed case: {source}")
        output.append(row)
    assert len(output) == 261
    target = OUT / "legacy_pronunciation_repairs.json"
    target.write_text(json.dumps(output, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    manifest = {
        "source_report": ".build/evaluation/pronunciation-impossible.json",
        "source_report_sha256": hashlib.sha256(REPORT.read_bytes()).hexdigest(),
        "reviewed_cases": len(output), "action_counts": dict(Counter(r["action"] for r in output)),
        "sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
        "canonical_dictionaries_modified": False,
        "review_method": "Each source row and each finite-family suffix read by a contributor; explicit syllables and semantic decisions; no pypinyin default-driven changes",
        "additional_recommended_keys": [
            {"file": "local_core_zh_pinyin.tsv", "word": "摩尔", "new_key": "moer", "reason": "保留国际单位符号mol别名，同时确保常规中文拼音moer可用。"},
            {"file": "local_core_zh_pinyin.tsv", "word": "电视盒", "new_key": "dianshihe", "reason": "保留旧英文混拼别名dianshibox，同时确保规范拼音dianshihe可用。"},
        ],
        "pronunciation_reference": "https://pedia.cloud.edu.tw/Entry/Detail?title=%E9%86%82",
    }
    target.with_suffix(".metadata.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, ensure_ascii=False))


if __name__ == "__main__":
    main()
