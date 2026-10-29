# Авторы и лицензии

## Оригинальный проект
**SA:COOP Friends** вдохновлён работой автора и основан на моде **SA:COOP** — кооперативе для GTA San Andreas
1.0 US. Это неофициальный форк.

- **Автор:** Akbar Karimov ([Karimov-Akbar](https://github.com/Karimov-Akbar))
- **Репозиторий:** https://github.com/Karimov-Akbar/sa-coop
- **Лицензия:** MIT, `Copyright (c) 2026 Karimov-Akbar`. Текст — в файле [LICENSE](LICENSE), он не изменён.
- **Новости автора:** [Telegram](https://t.me/cavalier_mods). Поддержать автора — по ссылке в
  [README автора](README.md).

Мы вдохновились работой автора и продолжаем её для игры с друзьями. Вся история коммитов оригинала сохранена:
код, архитектура, инструменты сборки, документация `docs/ARCHITECTURE.md`, изображения в `docs/images` и логотип
принадлежат автору и распространяются на условиях автора.

## Изменения форка
Изменения, сделанные в форке (начиная с коммита «SA:COOP Friends: база знаний, атрибуция и правовая информация»),
распространяются по той же лицензии MIT:

```
Copyright (c) 2026 участники SA:COOP Friends
```

Что именно изменено — в истории git и в [docs/fork/CHANGELOG.md](docs/fork/CHANGELOG.md).

## Код сообщества
Pull request'ы сообщества из репозитория автора (например, от [AmXLoVE](https://github.com/AmXLoVE)) переносятся
с сохранением авторства: `git cherry-pick` или строка `Co-authored-by:` в коммите и ссылка на исходный PR.

Уже перенесено:
- [PR №32](https://github.com/Karimov-Akbar/sa-coop/pull/32), AmXLoVE — выбор числа игроков и свои данные у каждого
  удалённого игрока (коммит `f5b9f0f`, в форке ограничено четырьмя игроками).

## Сторонний код
| Компонент | Лицензия | Где |
|---|---|---|
| [ENet](http://enet.bespin.org), © Lee Salzman | MIT | `third_party/enet/LICENSE` |
| [MinHook](https://github.com/TsudaKageyu/minhook), © Tsuda Kageyu | BSD 2-Clause | `third_party/minhook/LICENSE.txt` |
| [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) | своя | скачивается при сборке, в репозиторий не входит |
| [SilentPatch](https://github.com/CookiePLMonster/SilentPatch), © Adrian Zdanowicz (Silent) | MIT | `third_party/silentpatch/LICENSE`; `SilentPatchSA.asi` скачивается при сборке и кладётся в архив релиза без изменений |
| [Open Limit Adjuster](https://github.com/ThirteenAG/limit_adjuster_gta3vcsa), © ThirteenAG | MIT | `third_party/open-limit-adjuster/LICENSE`; `.asi` скачивается при сборке, настройки — `package/III.VC.SA.LimitAdjuster.ini` |

Адреса и структуры движка сверялись с открытыми проектами [gta-reversed](https://github.com/gta-reversed/gta-reversed)
и [plugin-sdk](https://github.com/DK22Pac/plugin-sdk) как со справочником; их код в репозиторий не копируется.

## Товарные знаки
Grand Theft Auto, GTA, San Andreas и Rockstar Games — товарные знаки их владельцев. Проект не связан с Rockstar
Games и Take-Two Interactive и не одобрен ими. Подробнее — в [LEGAL.md](LEGAL.md).
