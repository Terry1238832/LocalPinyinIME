# Architecture

LocalPinyinIME is split into three layers.

1. Windows TSF/COM integration

   The DLL exports `DllGetClassObject`, `DllCanUnloadNow`, `DllRegisterServer`,
   and `DllUnregisterServer`. `TextService` implements
   `ITfTextInputProcessorEx`, `ITfKeyEventSink`, and `ITfCompositionSink`.
   Activation stores the TSF thread manager and advises the keystroke sink.

2. Offline pinyin engine

   `PinyinEngine` owns a dictionary and a ranker. The first dictionary is small
   and embedded so the IME can run without network access or a large third-party
   engine.

3. Local data layer

   User frequency learning is represented by `UserLearning`. The current build
   uses a local UTF-8 tab-separated persistence file for tests. The SQLite
   adapter boundary is present in `SqliteDatabase` for the next milestone.

No layer reads the clipboard, sends network requests, or logs user input text.
