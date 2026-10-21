# Электроника и схемотехника

Лабораторные работы по дисциплине «Электроника и схемотехника», СПбПУ, 2025 год.
В репозитории собраны отчёты, схемы Multisim, таблицы измерений и исходники графиков.

Для просмотра отчёта прямо на GitHub откройте PDF. Рядом лежит редактируемый оригинал DOCX.

## Лабораторные работы

| № | Тема | Отчёт | Материалы |
| --- | --- | --- | --- |
| 1 | Основы работы в среде Multisim | [PDF](labs/lab-01/report.pdf) · [DOCX](labs/lab-01/report.docx) | [Папка](labs/lab-01/) |
| 2 | RL- и RC-цепи | [PDF](labs/lab-02/report.pdf) · [DOCX](labs/lab-02/report.docx) | [Папка](labs/lab-02/) |
| 3 | Электрические фильтры | [PDF](labs/lab-03/report.pdf) · [DOCX](labs/lab-03/report.docx) | [Папка](labs/lab-03/) |
| 4 | Спектры сигналов | [PDF](labs/lab-04/report.pdf) · [DOCX](labs/lab-04/report.docx) | [Папка](labs/lab-04/) |
| 5 | Полупроводниковые диоды | [PDF](labs/lab-05/report.pdf) · [DOCX](labs/lab-05/report.docx) | [Папка](labs/lab-05/) |
| 6 | Биполярные транзисторы | [PDF](labs/lab-06/report.pdf) · [DOCX](labs/lab-06/report.docx) | [Папка](labs/lab-06/) |
| 7 | Резистивный усилитель на транзисторе | [PDF](labs/lab-07/report.pdf) · [DOCX](labs/lab-07/report.docx) | [Папка](labs/lab-07/) |
| 8 | RC-автогенератор | [PDF](labs/lab-08/report.pdf) · [DOCX](labs/lab-08/report.docx) | [Папка](labs/lab-08/) |
| 9 | Логические элементы | [PDF](labs/lab-09/report.pdf) · [DOCX](labs/lab-09/report.docx) | [Папка](labs/lab-09/) |
| 10 | Триггеры, счётчики и регистры | [PDF](labs/lab-10/report.pdf) · [DOCX](labs/lab-10/report.docx) | [Папка](labs/lab-10/) |
| 11 | Цифровая обработка сигналов | [PDF](labs/lab-11/report.pdf) · [DOCX](labs/lab-11/report.docx) | [Папка](labs/lab-11/) |

## Структура

```text
.
├── labs/
│   ├── lab-01/
│   ├── ...
│   └── lab-11/
│       ├── report.docx       # Редактируемый отчёт
│       ├── report.pdf        # Версия для просмотра
│       ├── circuits/         # Схемы Multisim (.ms14)
│       └── data/             # Измерения и расчёты (.xlsx)
├── circuits/additional/      # Дополнительные схемы из исходной коллекции
├── docs/
│   └── laboratory-manual.pdf # Лабораторный практикум
├── scripts/
│   └── export-reports.ps1    # Экспорт DOCX в PDF через Microsoft Word
├── .gitattributes
├── .gitignore
└── README.md
```

В отдельных работах также есть `figures/` с изображениями и `plots/` с исходниками графиков `.agr`.
Подпапки создаются только там, где есть соответствующие материалы.
Дополнительные схемы находятся [здесь](circuits/additional/), лабораторный практикум — [здесь](docs/laboratory-manual.pdf).

## Как открыть материалы

- **PDF** — просмотр в браузере, на GitHub или в любом PDF-приложении.
- **DOCX** — редактирование в Microsoft Word; в других редакторах вёрстка может отличаться.
- **MS14** — схемы для NI Multisim 14.
- **XLSX** — таблицы для Microsoft Excel или совместимого редактора.
- **AGR** — исходники графиков для Grace / QtGrace.

## Обновление PDF

На Windows с установленным Microsoft Word запустите из корня репозитория:

```powershell
.\scripts\export-reports.ps1
```

Скрипт создаст недостающие PDF рядом с DOCX. Чтобы заново экспортировать все отчёты:

```powershell
.\scripts\export-reports.ps1 -Force
```

Документы открываются только для чтения, автоматическое обновление внешних ссылок отключено.
В исходных отчётах № 5 и № 11 есть OLE-ссылки на Excel по старым абсолютным путям.
Для обновления связанных объектов в Word укажите актуальные таблицы из `data/` соответствующей работы.

## Сохранность исходников

Содержимое исходных документов, схем, таблиц и изображений сохранено при переносе.
Резервные копии Multisim `Security copy` находятся локально в `.local/backups/` и исключены из Git.
В `.local/file-mapping.json` сохранены исходные пути, новые пути и SHA-256 всех перенесённых файлов.

Лабораторный практикум хранится как справочный материал; права на него принадлежат правообладателям.
