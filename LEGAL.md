# Правовая информация

## Кратко
- **SA:COOP Friends** — **неофициальный некоммерческий фан-проект**, вдохновлённый работой автора и основанный
  на моде автора SA:COOP: форк проекта
  [Karimov-Akbar](https://github.com/Karimov-Akbar/sa-coop) по лицензии MIT.
- Проект **не связан** с Rockstar Games и Take-Two Interactive, не одобрен и не поддерживается ими.
- В репозитории и в релизах **нет файлов игры** и нет кода Rockstar. Для игры нужна своя легально купленная копия
  GTA San Andreas.
- Grand Theft Auto, GTA, San Andreas и Rockstar Games — товарные знаки их владельцев. Названия упоминаются только
  для того, чтобы указать, с какой игрой совместим мод.

## Что есть в репозитории
- Исходный код мода: код автора оригинала и изменения форка ([NOTICE.md](NOTICE.md)).
- Сторонние библиотеки с открытыми лицензиями (ENet, MinHook) вместе с текстами их лицензий.
- Документация и скриншоты автора оригинала в `docs/`.

## Чего в репозитории нет и не будет
- Файлов игры: `gta_sa.exe`, архивов `*.img`, скриптов `main.scm` и `script.img`, моделей, текстур, звука, текстов
  игры, сохранений.
- Декомпилированного или утёкшего исходного кода Rockstar, файлов символов игры (`gta_sa.pdb` и выгрузок из него).
- Средств обхода защиты от копирования, ссылок на пиратские копии игры и инструкций, как их получить.
- Платных функций. Мод бесплатный, форк не продаёт сборки и не собирает пожертвования.

Это проверяет [`tools/fork/guard_check.py`](tools/fork/guard_check.py) в CI на каждый push и PR.

## Правила для участников
1. Не добавляйте файлы игры и данные, извлечённые из неё (таблицы из `*.dat`, `*.ide`, `*.ipl`, тексты `*.gxt`
   и т. п.). Мод читает их из папки игры у пользователя во время работы.
2. Не копируйте код из декомпиляции игры и из утечек. Допустимо, как в SA-MP, MTA и plugin-sdk: адреса функций и
   полей, имена из открытых проектов, собственный код, который их вызывает.
3. Инструменты из `tools/re/` работают только с копией игры, которая уже есть у разработчика. Их входные и
   выходные файлы (`re/`, `build/re/`) не попадают в git: это закрыто в `.gitignore`.
4. Скриншоты — только свои и только для документации, без вырезанных из игры ассетов.
5. Чужой код — только с совместимой лицензией и с указанием автора. PR сообщества переносим с сохранением
   авторства ([NOTICE.md](NOTICE.md)).
6. Не выдавайте форк за оригинальный SA:COOP: в релизах, описаниях и сообщениях пишите «SA:COOP Friends —
   неофициальный форк SA:COOP от Karimov-Akbar» и давайте ссылку на проект автора.

## Отношения с автором оригинала
- Лицензия MIT разрешает форк и распространение изменённой версии при сохранении копирайта и текста лицензии.
  Файл [LICENSE](LICENSE) автора сохранён без изменений, история коммитов автора — тоже.
- Форк развивается самостоятельно. Исправления, перенесённые из версий автора, сохраняют ссылку на его коммит
  (`git cherry-pick -x`).
- Ошибки форка не отправляем в репозиторий автора: для них есть Issues этого репозитория.
- Если автор попросит изменить название форка, убрать логотип автора или что-то ещё в оформлении, — делаем.

## Обращения правообладателей
Если вы считаете, что материал в репозитории нарушает ваши права, откройте issue с пометкой `legal` или
свяжитесь с владельцем репозитория через контакты в профиле владельца на GitHub. Спорный материал убираем из текущей
версии без споров, как только получим обращение, а затем разбираемся.

---

## English summary
**SA:COOP Friends** is an **unofficial, non-commercial fan project** inspired by and based on the MIT-licensed
SA:COOP mod by
[Karimov-Akbar](https://github.com/Karimov-Akbar/sa-coop). It is **not affiliated with, endorsed or supported by
Rockstar Games or Take-Two Interactive**. The repository and its releases contain **no game files, no game assets
and no Rockstar source code** (decompiled or leaked), and no copy-protection circumvention. Players need their own
legally purchased copy of GTA San Andreas. Grand Theft Auto, GTA, San Andreas and Rockstar Games are trademarks of
their respective owners and are used only to state compatibility. Rights holders: please open an issue labelled
`legal` or contact the repository owner via their GitHub profile; disputed material is removed promptly.
