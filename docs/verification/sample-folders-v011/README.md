# MOSS 0.11 · folders and four-part samples

Проверено 18 ноября 2026 на оригинальной RG35XX / GarlicOS 1.4.9, штатные 1.008 GHz, 32 kHz, 1024 стереофрейма. Установленная сборка и сохранность файлов: [summary.json](summary.json), [deployment.json](deployment.json).

## Что проверено

- Полный существующий desktop suite прошёл после изменения каталога, PCM-представлений и формата MS17: секвенсор, one-shot, параметры, локи, свинг, SONG, старые сохранения, синтезаторы и эффекты. После трёх правок UI заново прошёл целевой сценарий `sample_folders_test` и собраны финальные ARM/Windows приложения.
- Новый сценарий проверяет вложенные каталоги и возврат фокуса, страницы, четыре предпросмотра, отсутствие случайного проигрывания после изменения, мелкое/крупное движение, удерживаемую смену MOVE/TRIM, именование, сохранение/повторное открытие набора, запрет перезаписи и выхода из каталога через имя, четыре назначения с Undo, ошибку чтения без изменения назначений, каталог больше 32 файлов, стабильные ID при изменении порядка папок, сохранение ID >31, предел 1 МиБ на часть и 32 МиБ на кэш. Он выполнен также на ARM: [device-unit.txt](device-unit.txt). ARM unit предшествовал трём финальным правкам интерфейса; их дополнительно проверили desktop-тестом и реальными событиями кнопок на устройстве.
- Шесть коротких нагрузочных прогонов по 10 секунд: суммарно **60 секунд / 1884 callbacks**. Четыре дорожки, максимальные паттерны, эффекты дорожек и мастера, 32 МиБ PCM; смешанные инструменты, грануляр, ORGANISM и четыре сэмплера. Во всех этих прогонах `over_budget`, `late_intervals`, `stream_underruns`, `schedule_overflows`, `audio_realtime_error` равны нулю. [v28](v28/manifest.json), [v29](v29/manifest.json).
- Финальный бинарник после правок интерфейса отдельно проверен 20 секунд на грануляре и сэмплерах: максимум **29.748 мс при бюджете 32 мс**, без указанных ошибок. [v30](v30/manifest.json). Это короткие проверки, не утверждение о любой продолжительности и любой SD-карте.
- В отдельной тестовой установке физическими событиями кнопок пройден путь: папки → WAV → четыре части → сдвиг части 2 с 1000 до 1101 мс → B предпросмотр → длительное L1+right в TRIM → имя → четыре WAV → Y назначение. Подтверждены `PART 2 PLAY / B STOP`, выбранная дорожка и четыре слота. Обычный выход вернулся в Garlic с кодом 0.

Интерактивная сессия с USB-контролем и остановками/перезапусками предпросмотра отдельно записала **5 увеличенных интервалов между callbacks**, при этом ни одного превышения времени обработки аудиоблока и ни одного опустошения потока; максимум обработки 3.505 мс. Причина этих пяти интервалов отдельно не установлена. Этот сеанс не засчитывается как нагрузочный PASS: [interactive-ui.json](interactive-ui.json). Чтение/назначение/сохранение файлов по-прежнему останавливает транспорт; время подготовки файлов не является обещанием непрерывного воспроизведения.

## Установка и сохранность

Обновлены только `Roms/APPS/MOSS/moss` и `MOSS.sh`. Предыдущие бинарник и launcher сохранены в `/mnt/mmc/MOSS-Backups/20261118-sample-folders-v011`; проекты и библиотека дополнительно скопированы на компьютер. Все **71 пользовательский файл** проектов и библиотеки совпали по SHA-256 до обновления и после обычного запуска/выхода установленной 0.11. Новые тестовые наборы находятся только в отдельной тестовой папке, не в пользовательской библиотеке.

Проверен штатный запуск до ATLAS с существующими секвенциями. Затем MOSS закрыт, запись синхронизирована, восстановлен governor `interactive`, консоль оставлена в Garlic. GitHub в этой задаче не обновлялся.

Визуальная проверка: [report](../../../.impeccable/review/sample-folders-v011/review.md), [контракт](../../../design/current/native-sample-folders-v011.md). SHIP относится к трём исправленным замечаниям. Дополнительные `device-*.png` и `installed-atlas.png` — реальные framebuffer-снимки после этой проверки.

## English

Verified on original RG35XX / GarlicOS 1.4.9 at stock 1.008 GHz, 32 kHz / 1024 frames. The full existing desktop suite passed after catalog/PCM-view/MS17 changes; the scoped folder test passed again after final UI corrections. The ARM filesystem test passed, and real button events confirmed the final MOVE/TRIM, audition, naming and four-slot assignment flows.

Six 10-second load runs, 1884 callbacks total, passed with zero processing deadline overruns, late intervals, stream underruns or schedule overflows. The exact installed binary received an additional final 20-second granular/sampler qualification within those 60 seconds: maximum callback 29.748 ms against 32 ms. This is bounded evidence, not an indefinite reliability guarantee.

The separate interactive USB session recorded five late callback intervals, zero processing overruns and zero stream underruns; the cause was not isolated. It is not counted as a load-test pass. File preparation intentionally stops transport.

The installed binary was read back and hash-verified. All 71 existing user project/library files remained byte-identical after normal installed launch and exit. Test data stayed in an isolated directory. Previous executable/launcher and user data are backed up. MOSS exited normally; filesystem sync completed and Garlic's interactive CPU governor was restored. No GitHub publication was performed.
