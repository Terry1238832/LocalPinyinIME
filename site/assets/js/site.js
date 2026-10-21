// 本地拼音 — website behavior. Spec: site/DESIGN.md §7.
// No dependencies. Every demo has a static end state in the HTML, so the page
// reads the same without this script or with reduced motion.
(() => {
  'use strict';

  const root = document.documentElement;
  const $ = (selector, scope = document) => scope.querySelector(selector);
  const $$ = (selector, scope = document) => Array.from(scope.querySelectorAll(selector));
  const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
  const clamp = (value, low = 0, high = 1) => Math.min(high, Math.max(low, value));

  const reducedQuery = matchMedia('(prefers-reduced-motion: reduce)');
  let reduced = reducedQuery.matches;
  const connection = navigator.connection || {};
  const lowPower = Boolean(connection.saveData) ||
    (navigator.hardwareConcurrency > 0 && navigator.hardwareConcurrency <= 4) ||
    (navigator.deviceMemory > 0 && navigator.deviceMemory <= 4);
  root.classList.toggle('reduced', reduced);

  // ── Theme: auto → light → dark ─────────────────────────────────────────
  const themeQuery = matchMedia('(prefers-color-scheme: dark)');
  const themeLabels = { auto: '外观：跟随系统', light: '外观：浅色', dark: '外观：深色' };
  const themeListeners = [];
  const currentTheme = () => root.dataset.theme === 'dark' ? 'dark' : 'light';

  function applyTheme(choice) {
    const dark = choice === 'dark' || (choice === 'auto' && themeQuery.matches);
    root.dataset.theme = dark ? 'dark' : 'light';
    root.dataset.themeChoice = choice;
    $$('[data-theme-color]').forEach(meta => { meta.content = dark ? '#000000' : '#ffffff'; });
    const toggle = $('[data-theme-toggle]');
    if (toggle) toggle.setAttribute('aria-label', themeLabels[choice]);
    themeListeners.forEach(listener => listener(currentTheme()));
  }
  function storedTheme() {
    try { return localStorage.getItem('lp-theme') || 'auto'; } catch { return 'auto'; }
  }
  $('[data-theme-toggle]')?.addEventListener('click', () => {
    const order = ['auto', 'light', 'dark'];
    const next = order[(order.indexOf(root.dataset.themeChoice || 'auto') + 1) % order.length];
    try { localStorage.setItem('lp-theme', next); } catch { /* private mode: keep it for this visit */ }
    applyTheme(next);
  });
  themeQuery.addEventListener('change', () => { if ((root.dataset.themeChoice || 'auto') === 'auto') applyTheme('auto'); });
  applyTheme(storedTheme());

  // ── The candidate window ───────────────────────────────────────────────
  const gear = '<svg class="cand-gear" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.3" d="M8 5.6a2.4 2.4 0 1 0 0 4.8 2.4 2.4 0 0 0 0-4.8zM6.9 1.5h2.2l.4 1.8 1.3.7 1.7-.7 1.1 1.9-1.4 1.2v1.2l1.4 1.2-1.1 1.9-1.7-.7-1.3.7-.4 1.8H6.9l-.4-1.8-1.3-.7-1.7.7-1.1-1.9 1.4-1.2V6.4L2.4 5.2l1.1-1.9 1.7.7 1.3-.7z"/></svg>';
  const escape = text => text.replace(/[&<>"]/g, ch => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[ch]);
  const pinyinHtml = pinyin => pinyin.split("'").map(escape).join("<i>'</i>");

  // Renders one page of candidates. `previous` (the words shown before) marks
  // what changed: those candidates rise into place and, in Split-Flap, only
  // the changed tiles flip. `enter` replays the style's own arrival.
  function renderCandidates(el, { pinyin, words, previous = null, enter = false }) {
    const style = el.dataset.candStyle;
    let tile = 0;
    const items = words.slice(0, 9).map((word, index) => {
      const changed = !previous || previous[index] !== word;
      let inner = escape(word);
      if (style === 'flap') {
        const before = previous ? Array.from(previous[index] || '') : [];
        inner = Array.from(word).map((ch, at) => {
          const flip = !previous || before[at] !== ch || index === 0;
          return `<span class="cand-tile${flip ? ' is-flip' : ''}" style="--t:${tile++}">${escape(ch)}</span>`;
        }).join('');
      }
      const textAttr = style === 'marker' && index === 0 ? ` data-text="${escape(word)}"` : '';
      return `<li class="cand-item${index === 0 ? ' is-focus' : ''}${changed && previous ? ' is-new' : ''}" style="--i:${index}">` +
        `<span class="cand-c"><span class="cand-n">${index + 1}</span><span class="cand-w"${textAttr}>${inner}</span></span>` +
        (index === 0 ? '<span class="cand-hint">空格</span>' : '') + '</li>';
    }).join('');
    el.innerHTML =
      `<div class="cand-q"><span class="cand-mark"></span><span class="cand-py">${pinyinHtml(pinyin)}<b class="cand-caret"></b></span>` +
      `<span class="cand-sep"></span><span class="cand-ctl"><span class="cand-dots"><i class="on"></i><i></i></span>${gear}</span></div>` +
      `<ol class="cand-list">${items}</ol>`;
    if (enter && !reduced) {
      el.classList.remove('is-pour');
      void el.offsetWidth;
      el.classList.add('is-pour');
    }
  }

  // Candidate windows outside the stage follow the page's theme.
  const themedWindows = $$('.hero-cand, .context-cand');
  const setTone = theme => themedWindows.forEach(el => { el.dataset.candTone = theme; });
  themeListeners.push(setTone);
  setTone(currentTheme());

  // ── Reveals ────────────────────────────────────────────────────────────
  const onReveal = new Map();
  const revealObserver = new IntersectionObserver(entries => {
    for (const entry of entries) {
      if (!entry.isIntersecting) continue;
      entry.target.classList.add('is-in');
      revealObserver.unobserve(entry.target);
      onReveal.get(entry.target)?.();
    }
  }, { rootMargin: '0px 0px -10% 0px', threshold: .15 });
  function reveal(el, callback) {
    if (callback) onReveal.set(el, callback);
    if (reduced) { el.classList.add('is-in'); callback?.(); return; }
    revealObserver.observe(el);
  }
  // With reduced motion everything is simply there.
  const revealAll = () => $$('[data-reveal]').forEach(el => el.classList.add('is-in'));
  if (reduced) revealAll();
  $$('[data-reveal]').forEach(el => { if (!el.matches('.xform, .style-stage, .en-demo, [data-compose]')) reveal(el); });

  // Typing animations run one at a time, in the order they come into view.
  let typingQueue = Promise.resolve();
  const enqueue = job => { typingQueue = typingQueue.then(job).catch(() => undefined); return typingQueue; };

  // ── Composition: pinyin typed above a title ─────────────────────────────
  async function typeComposition(el) {
    const run = $('.compose-py', el);
    const syllables = el.dataset.compose.split("'");
    const steps = [];
    syllables.forEach((syllable, index) => {
      for (let length = 1; length <= syllable.length; length++)
        steps.push([...syllables.slice(0, index), syllable.slice(0, length)].join("'"));
    });
    for (const step of steps) { run.innerHTML = pinyinHtml(step); await sleep(70 + Math.random() * 50); }
  }

  // ── 1 · Hero ────────────────────────────────────────────────────────────
  const hero = $('.hero');
  const heroWindow = $('.hero-cand');
  const docComp = $('.hero .doc-comp');
  let heroVisible = true;
  new IntersectionObserver(([entry]) => { heroVisible = entry.isIntersecting; }, { threshold: .1 }).observe(hero);
  const waitUntilShown = async () => { while (!heroVisible || document.hidden) await sleep(400); };

  // What the window lists as the pinyin grows; between these it keeps the last list.
  const heroStates = [
    ['n', "n", ['你', '那', '年', '呢', '能', '女', '拿', '难', '内']],
    ['ni', "ni", ['你', '尼', '泥', '逆', '拟', '妮', '倪', '腻', '匿']],
    ['nih', "ni'h", ['你好', '你会', '你还', '你和', '拟合', '你', '尼', '泥', '逆']],
    ['niha', "ni'ha"],
    ['nihao', "ni'hao", ['你好', '拟好', '你', '泥', '尼', '妮', '逆', '倪', '腻']],
    ['nihaos', "ni'hao's"],
    ['nihaosh', "ni'hao'sh"],
    ['nihaoshi', "ni'hao'shi"],
    ['nihaoshij', "ni'hao'shi'j", ['你好世界', '你好', '拟好', '你', '泥', '尼', '妮', '逆', '倪']],
    ['nihaoshiji', "ni'hao'shi'ji"],
    ['nihaoshijie', "ni'hao'shi'jie"],
  ];

  async function heroTyping() {
    let words = null;
    for (const [raw, pinyin, list] of heroStates) {
      await waitUntilShown();
      docComp.textContent = raw;
      const next = list || words;
      renderCandidates(heroWindow, { pinyin, words: next, previous: words, enter: !words });
      heroWindow.classList.remove('is-hidden');
      words = next;
      await sleep(115 + Math.random() * 70);
    }
  }

  async function startHero() {
    const compose = $('.hero [data-compose]');
    if (reduced) { root.classList.add('is-ready'); return; }
    heroWindow.classList.add('is-hidden');
    docComp.textContent = '';
    const name = $('.compose-py', compose);
    name.innerHTML = '';
    await sleep(200);
    compose.classList.add('is-typing');
    await typeComposition(compose);
    root.classList.add('is-ready');
    await sleep(500);
    // One pass, resting on the composed window: the whole sequence stays
    // under five seconds.
    await heroTyping();
  }

  // ── 3 · Context ─────────────────────────────────────────────────────────
  const contextExamples = [
    { before: '他跑', raw: 'de', pinyin: 'de', words: ['得', '的', '地', '德', '嘚'] },
    { before: '没有', raw: 'shijian', pinyin: "shi'jian", words: ['时间', '事件', '实践', '世间', '时', '是', '事'] },
    { before: '这种又是', raw: 'zalaide', pinyin: "za'lai'de", words: ['咋来的', '杂来的', '咋', '杂', '砸', '扎'] },
  ];
  const contextScene = $('.context');
  const contextWindow = $('.context-cand');
  const contextBefore = $('[data-ctx]');
  const contextPinyin = $('[data-ctx-py]');
  const contextSteps = $$('.context-steps li');
  let contextStep = -1;

  // The progress marks double as buttons: each scrolls to its example.
  contextSteps.forEach((mark, index) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'context-step';
    const example = contextExamples[index];
    button.setAttribute('aria-label', `例子 ${index + 1}：“${example.before}”后面输入 ${example.raw}`);
    button.addEventListener('click', () => {
      if (reduced || !contextScene.offsetHeight) { showContext(index); return; }
      const top = contextScene.getBoundingClientRect().top + scrollY;
      const travel = contextScene.offsetHeight - innerHeight;
      scrollTo({ top: top + travel * (index + .5) / contextExamples.length, behavior: 'smooth' });
    });
    mark.append(button);
  });
  $('.context-steps')?.removeAttribute('aria-hidden');

  async function showContext(index) {
    if (index === contextStep) return;
    const first = contextStep < 0;
    contextStep = index;
    contextSteps.forEach((mark, at) => {
      mark.classList.toggle('on', at === index);
      mark.firstElementChild?.setAttribute('aria-pressed', String(at === index));
    });
    const example = contextExamples[index];
    if (!first && !reduced) { contextBefore.classList.add('is-swap'); await sleep(160); }
    if (contextStep !== index) return;
    contextBefore.textContent = example.before;
    contextPinyin.textContent = example.raw;
    contextBefore.classList.remove('is-swap');
    renderCandidates(contextWindow, { pinyin: example.pinyin, words: example.words, enter: true });
  }
  showContext(0);

  // ── 4 · Typing rows ─────────────────────────────────────────────────────
  let nextRowStart = 0;
  $$('.xform').forEach(row => {
    const input = $('.x-in', row);
    const text = input.textContent;
    if (reduced) { row.classList.add('is-done'); return; }
    input.textContent = '';
    reveal(row, async () => {
      // Rows that come into view together start a beat apart and overlap.
      const start = Math.max(performance.now(), nextRowStart);
      nextRowStart = start + 260;
      await sleep(start - performance.now());
      input.classList.add('is-typing');
      for (let length = 1; length <= text.length; length++) { input.textContent = text.slice(0, length); await sleep(28 + Math.random() * 26); }
      await sleep(140);
      input.classList.remove('is-typing');
      row.classList.add('is-done');
    });
  });

  // ── 5 · Candidate styles ────────────────────────────────────────────────
  const styleDescriptions = {
    flow: '当前候选是一滴流动的液体：换候选时拉长、收腰，再聚拢流过去；窗口按候选的长短在紧凑、平衡、展开三种形态之间切换。这是默认风格。',
    capsule: '整个窗口是一条圆角长条：拼音在左，候选随后，当前候选是一枚实心胶囊。它是最矮的候选窗口。',
    flap: '机场的航班牌：每个字一块翻页牌，当前候选是一排琥珀色的牌；候选变了，变了的字一块接一块翻过来。',
    marker: '一张笔记纸：当前候选被荧光笔划上一道，笔触跟着指针走，换词时重新划一笔。',
    type: '让汉字做主角：字号、字重和一道朱红短线区分主次，没有按钮感。',
    rail: '候选排在一条轨道上，焦点线随指针滑动；紧凑精准，适合高频输入。',
    glass: '分层的悬浮面板，当前候选是一枚陶瓷质感的实体键帽。',
    column: '候选一行一个，像菜单一样从上往下读；长词和整句都能完整显示。',
  };
  const styleSample = { pinyin: "sui'bian", words: ['随便', '岁', '随', '碎', '虽', '遂', '穗', '隋', '髓'] };
  const styleWindow = $('.style-cand');
  const styleState = { style: 'flow', tone: 'light' };

  // Like the IME, a row that does not fit shows fewer candidates per page;
  // only below four does the window shrink.
  function showStyle(enter) {
    styleWindow.dataset.candStyle = styleState.style;
    styleWindow.dataset.candTone = styleState.tone;
    styleWindow.style.removeProperty('--k');
    const stage = styleWindow.parentElement;
    const room = stage.clientWidth;
    let count = styleSample.words.length;
    renderCandidates(styleWindow, { pinyin: styleSample.pinyin, words: styleSample.words, enter });
    // Adaptive Flow wraps its row and Column List stacks, so only the
    // single-row styles page.
    const singleRow = !['flow', 'column'].includes(styleState.style);
    while (singleRow && styleWindow.scrollWidth > room && count > 4) {
      count -= 1;
      renderCandidates(styleWindow, { pinyin: styleSample.pinyin, words: styleSample.words.slice(0, count), enter });
    }
    if (styleWindow.scrollWidth > room) {
      const k = parseFloat(getComputedStyle(styleWindow).getPropertyValue('--k')) || 1;
      styleWindow.style.setProperty('--k', (k * room / styleWindow.scrollWidth * .98).toFixed(3));
    }
    const description = $('[data-style-desc]');
    if (description) description.textContent = styleDescriptions[styleState.style];
  }

  $('[data-style-picker]').addEventListener('change', event => { styleState.style = event.target.value; showStyle(true); });
  $('[data-tone-picker]').addEventListener('change', event => { styleState.tone = event.target.value; showStyle(false); });
  showStyle(false);
  reveal($('.style-stage'), () => showStyle(true));

  // ── 8 · English ─────────────────────────────────────────────────────────
  const englishLine = $('[data-en]');
  const englishTyped = 'i think teh plan could of worked';
  const englishFixed = [['I', true], ['think'], ['the', true], ['plan'], ['could'], ['have', true], ['worked']];
  const englishResult = () => englishFixed.map(([word, fixed]) => `<span class="en-word${fixed ? ' is-fixed' : ''}">${word}</span>`).join(' ');
  if (englishLine) {
    englishLine.innerHTML = englishResult();
    const demo = $('.en-demo');
    if (!reduced) {
      englishLine.textContent = '';
      reveal(demo, () => enqueue(async () => {
        const keys = $('.keys', demo);
        for (let length = 1; length <= englishTyped.length; length++) {
          englishLine.innerHTML = `${escape(englishTyped.slice(0, length))}<span class="en-caret"></span>`;
          await sleep(45 + Math.random() * 40);
        }
        await sleep(500);
        keys.classList.add('is-pressed');
        await sleep(520);
        englishLine.innerHTML = englishResult() + '<span class="en-caret"></span>';
      }));
    }
  }

  // ── 10 · Download composition ───────────────────────────────────────────
  $$('.download [data-compose]').forEach(compose => {
    if (reduced) return;
    const run = $('.compose-py', compose);
    run.innerHTML = '';
    reveal(compose, () => enqueue(() => typeComposition(compose)));
  });

  // ── Scroll-driven scenes ────────────────────────────────────────────────
  const heroCopy = $('.hero-copy');
  const heroStage = $('.hero-stage');
  const manifesto = $('.manifesto');
  const manifestoWords = $$('[data-words] span');
  const zoomFigures = $$('[data-zoom]');
  const nav = $('.nav');
  const stages = $$('.stage');
  let pending = false;

  const sceneProgress = (rect, offset = 0) => {
    const travel = rect.height - (innerHeight - offset);
    return travel > 0 ? clamp((offset - rect.top) / travel) : 0;
  };

  // One frame: read every rectangle first, then write, so the browser lays
  // out once.
  function update() {
    pending = false;
    const viewport = innerHeight;
    const overStage = stages.some(stage => { const rect = stage.getBoundingClientRect(); return rect.top <= 24 && rect.bottom >= 24; });
    const animate = !reduced;
    const heroProgress = animate && !lowPower ? clamp(scrollY / Math.max(1, hero.offsetHeight)) : 1;
    const manifestoProgress = animate && manifesto ? sceneProgress(manifesto.getBoundingClientRect()) : 1;
    const contextProgress = animate && contextScene ? sceneProgress(contextScene.getBoundingClientRect(), 48) : null;
    const zooms = animate && !lowPower ? zoomFigures.map(figure => figure.getBoundingClientRect()) : [];

    if (overStage) nav.dataset.navTone = 'dark'; else delete nav.dataset.navTone;
    if (!animate) return;
    if (heroProgress < 1) {
      heroCopy.style.opacity = String(1 - clamp(heroProgress * 1.6));
      heroCopy.style.transform = `translateY(${(-heroProgress * 48).toFixed(1)}px)`;
      heroStage.style.transform = `translateY(${(-heroProgress * 72).toFixed(1)}px) scale(${(1 + heroProgress * .06).toFixed(4)})`;
    }
    const lit = Math.round(clamp(manifestoProgress * 1.25) * manifestoWords.length);
    manifestoWords.forEach((word, index) => word.classList.toggle('lit', index < lit));
    if (contextProgress !== null) showContext(Math.min(contextExamples.length - 1, Math.floor(contextProgress * contextExamples.length)));
    zooms.forEach((rect, index) => {
      if (rect.bottom < 0 || rect.top > viewport) return;
      const progress = clamp((viewport - rect.top) / (viewport * .85));
      const eased = 1 - Math.pow(1 - progress, 3);
      zoomFigures[index].style.setProperty('--zoom', (.88 + .12 * eased).toFixed(4));
    });
  }
  const schedule = () => { if (!pending) { pending = true; requestAnimationFrame(update); } };
  addEventListener('scroll', schedule, { passive: true });
  addEventListener('resize', schedule, { passive: true });
  // Re-page the style window when the width changes (not when a phone's
  // address bar changes the height).
  let resizeTimer = 0;
  let lastWidth = innerWidth;
  addEventListener('resize', () => {
    if (innerWidth === lastWidth) return;
    lastWidth = innerWidth;
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => showStyle(false), 120);
  }, { passive: true });

  reducedQuery.addEventListener('change', () => {
    reduced = reducedQuery.matches;
    root.classList.toggle('reduced', reduced);
    if (reduced) {
      revealAll();
      heroCopy.removeAttribute('style');
      heroStage.removeAttribute('style');
      zoomFigures.forEach(figure => figure.style.removeProperty('--zoom'));
      manifestoWords.forEach(word => word.classList.add('lit'));
    }
    schedule();
  });

  update();
  startHero();
})();
