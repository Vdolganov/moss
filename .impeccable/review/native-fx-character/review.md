# Native FX visual review · 0.6.2-dev

Reviewed generated images at the actual 640×480 logical resolution:

- `newfx-chain.png`: all five named effects visible, selected DRIVE row and ON/BYPASS states fit; two-active cap and reorder hint visible.
- `newfx-drive.png`: four CORE controls fit the fixed Y/B/A/X fields.
- `newfx-character.png`: CORE/CHARACTER tabs, BIAS sign, curve name and neutral transfer model are readable.
- `newfx-orbit.png`: OFF is explicit at LOW CUT zero; HIGH CUT 9000 and Hz units fit; MOTION remains the fourth field.
- `newfx-locks.png`: FX name and bank 2/2 visible; inherited fields use `--`; nested navigation is shown in the footer.

No new palette, typeface, shadows, glow or decorative motion. The transfer curve is a model, not measured output. TRACK and MASTER reuse the same effect panel and controls. Automated interaction tests cover both master and track bank navigation and physical-button parameter edits.

Проверены список из пяти эффектов, оба банка, подписи физических кнопок и FX LOCKS. Новые цвета и декоративные элементы не добавлены. Это визуальная проверка; аппаратные измерения хранятся отдельно в `docs/verification/fx-character-v1`.

Actual device captures confirm the library version, effect chain, DRIVE CHARACTER, ORBIT CHARACTER, BIAS lock 20 in DRIVE bank 2/2, and MASTER FX switching between CORE/CHARACTER. The application exited through held MENU with exit code 0, returned to Garlic and restored the interactive governor. Original user project hashes were rechecked after the session; the newly added demo was restored to its generated settings.
