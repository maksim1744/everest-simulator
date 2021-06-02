# everest-simulator

Для сборки необходим компилятор C++ с поддержкой версии C++ не ниже 17. Компиляция производится с использованием `g++`.

Запуск производится командой `main.exe scheduler_name workflow.json resources.json settings.json [failures.json]`.
Здесь `scheduler_name` это алгоритм для составления расписания (heft, greedy или adaptive). `workflow.json`, `resources.json`, `settings.json`, `failures.json` (опциональный) содержат соответственно информацию о workflow, ресурсах, общих настройках и запланированных отказах. Примеры файлов находятся в папке [files](/files).
