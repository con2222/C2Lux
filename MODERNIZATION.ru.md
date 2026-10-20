# Обновление WebGPU и план самостоятельной разработки

## Что изменено

Обновление сохраняет классы `App`, `Renderer`, `Window`, `Camera`, `Mesh`,
`ResourceManager` и `UILayer`. Загрузка OBJ, существующие шейдеры и два прохода
рендеринга — модель с MSAA 4×, затем интерфейс — остаются основой проекта.
Ниже отдельно перечислены сделанные изменения и упражнения на будущее.

### Зависимости и CMake

| Компонент | Было | Стало |
| --- | --- | --- |
| Dawn | `chromium/6536` | `v20261016.190333` |
| Dear ImGui | локальная копия `1.89.8` | `v1.93.0` через FetchContent |
| C++ | 17 | 20, требуется современным Dawn |
| Минимальный CMake | 3.20 | 3.22, требуется выбранным Dawn |

- Версия Dawn закреплена в `vendor/webgpu/cmake/FetchDawn.cmake`; параметр
  `C2LUX_DAWN_TAG` позволяет явно выбрать другую версию. При смене версии нужно
  заново проверить совместимость API и backend ImGui.
- Используется `FetchContent_MakeAvailable`, без ручных `git reset`, устаревшего
  `FetchContent_Populate` и подмены upstream-скрипта загрузки зависимостей.
  Автоматическая загрузка всех Git submodules отключена: зависимости выбирает
  собственный скрипт Dawn.
- Для Dawn отключены тесты, примеры и лишние инструменты Tint. На Windows
  сохранён Vulkan по умолчанию, на macOS — Metal, на Linux — Vulkan.
  D3D12 можно включить явно через `-DDAWN_ENABLE_D3D12=ON` с необходимыми SDK/DXC.
  Из default build исключены утилиты Dawn, от которых приложение не зависит.
  Поддержка поверхностей WinUI/UWP отключена: GLFW использует desktop HWND.
- Отключён тестовый Null backend, чтобы отсутствие рабочего GPU не маскировалось
  успешным запуском без отрисовки. На Windows включена загрузка системных GPU DLL:
  в данном релизе загрузка `vulkan-1.dll` без абсолютного пути при стандартных
  настройках давала ошибку Windows 87. Выбранный адаптер выводится в консоль.
- ImGui core и его backend берутся из одного тега. Старые исходники в
  `vendor/imgui` сохранены, но больше не компилируются. Это позволяет избежать
  большого обновления тысяч строк стороннего кода в репозитории.
- `vendor/webgpu` подключается до зависящих от него библиотек; заголовки Dawn
  распространяются через CMake target, без привязки к внутренней папке `_deps`.
- Определения X11/Wayland ограничены адаптером GLFW–WebGPU. Исправлен аргумент
  target в вспомогательной функции `utils.cmake`.
- Для GLM включён `GLM_FORCE_DEPTH_ZERO_TO_ONE`: проекция соответствует диапазону
  глубины WebGPU `[0, 1]`. `GLFW_INCLUDE_NONE` убирает ненужные OpenGL-заголовки.
- Шейдеры, шрифты и ресурсы копируются рядом с исполняемым файлом после сборки,
  в том числе в `Release` при использовании Visual Studio.
- В существующей ветке Emscripten старый `-sUSE_WEBGPU` заменён на порт
  `--use-port=emdawnwebgpu`. Для него нужен Emscripten 4.0.10 или новее.
  Браузерный запуск отдельно не проверен; существующие синхронная инициализация,
  цикл приложения и native file dialog потребуют отдельной адаптации для Web.

### Код приложения

- Загрузка WGSL использует `wgpu::ShaderSourceWGSL` вместо
  `ShaderModuleWGSLDescriptor`.
- Callbacks адаптера, устройства и диагностики шейдеров переведены на новый
  C++ API с `wgpu::StringView`. Сообщения выводятся с учётом длины строки.
  Перед чтением compilation info проверяются статус и указатель.
- Потеря устройства и необработанные ошибки регистрируются через
  `DeviceDescriptor::SetDeviceLostCallback` и `SetUncapturedErrorCallback`.
  При потере устройства приложение прекращает рендеринг; автоматическое
  восстановление устройства пока не реализовано.
  Копирование и перемещение `Renderer` запрещены: persistent callbacks привязаны
  к его адресу.
- `Instance::ProcessEvents` вызывается в основном цикле, включая свёрнутое
  окно. Ожидание инициализации больше не крутит CPU без паузы.
- Удалены ручные списки устаревших лимитов и функций `setDefault`: используются
  актуальные значения C++ descriptors и стандартные лимиты устройства.
  Убраны прежние ограничения буфера в 150 000 байт и текстур в 2048 пикселей.
  Лимиты WebGPU по-прежнему действуют: для более крупных ресурсов нужно
  запросить необходимые значения, предварительно проверив `Adapter::GetLimits`.
- Адаптер оконной поверхности использует новые структуры `SurfaceSource*`
  и корректную инициализацию `WGPUSurfaceDescriptor`.
- Получение surface texture различает `SuccessOptimal`, `SuccessSuboptimal`,
  `Timeout`, `Outdated`, `Lost` и ошибку. Временный сбой пропускает кадр,
  устаревшая или потерянная поверхность переконфигурируется, ошибка останавливает
  цикл. При пропуске кадра также завершается начатый ImGui frame.
- Инициализация ImGui использует `ImGui_ImplWGPU_InitInfo`. Частичная
  инициализация и повторное освобождение UI обрабатываются корректно.
- Проверяются создание окна и результат `initWebGPU()`. GLFW завершается после
  уничтожения UI, GPU-ресурсов и окна. При resize больше нет второго рендера
  того же кадра, а при свёрнутом окне есть короткое ожидание событий.
- Пути к шейдерам и шрифтам действительно используют настройки CMake.
  Загрузка шейдера или OBJ больше не дописывает имя файла в сохранённый базовый
  путь, что важно для повторных загрузок и будущего hot reload.
- Uniform-структуры обнуляются перед заполнением, `indexCount` инициализирован,
  пустая модель не вызывает indexed draw. Размеры ёмкостей mesh-буферов используют
  `size_t`, рост рассчитывается целочисленно. Убран вывод камеры каждый кадр.
- В `vendor/tiny_obj_loader.h` добавлен `constexpr` к внутренней функции
  `tinyobj_ff::distance`: встроенный fast_float вызывает её из constexpr-кода.
  GCC 16 с C++20 и `-Werror` обнаружил эту несовместимость. Быстрый парсер сохранён.

## Как собирать

Нужны CMake 3.22+, Git, Python 3 и компилятор с C++20. Для Windows подходит
Visual Studio с компонентом Desktop development with C++ и Windows SDK.
Первая конфигурация загружает Dawn, его зависимости, GLFW, NFD и ImGui;
первая сборка Dawn может занять заметное время и место на диске.

Используй новую папку сборки, чтобы старый CMake cache не подмешал старый Dawn:

```powershell
cmake -S . -B build/modern
cmake --build build/modern --config Release --parallel
```

Для одноконфигурационного генератора, например Ninja, укажи конфигурацию сразу:

```sh
cmake -S . -B build/modern -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/modern --parallel
```

По умолчанию `DEV_MODE=ON`: шейдеры, шрифты и ресурсы читаются из исходного
дерева, поэтому WGSL можно редактировать без копирования файлов. Для переносимой
сборки добавь `-DDEV_MODE=OFF` и запускай программу из папки с executable,
рядом с `shaders`, `fonts` и `resources`.

На Linux нужны dev-пакеты выбранной оконной системы, а также зависимости NFD.
Для сборки только с X11: `-DUSE_X11=ON -DUSE_WAYLAND=OFF`;
для Wayland: `-DUSE_X11=OFF -DUSE_WAYLAND=ON`.

## Проверки обновления

- Полная сборка Release прошла с CMake 4.4 и GCC 16.1 (MSYS2 MinGW),
  backend Vulkan. Собранный executable в рабочем окружении:
  `build/modernization/cmake/app.exe`.
- Тест в скрытом окне прошёл на **AMD Radeon(TM) Graphics** с явной проверкой
  backend **Vulkan**: создание pipeline и UI, повторная загрузка WGSL и OBJ,
  отрисовка модели, пустой mesh, resize, пересборка шрифтов и повторное
  освобождение UI. Ошибок WebGPU при этих проверках не было.
- Все восемь `.cpp` приложения проверены GCC 16.1 с `-std=c++20 -Werror`
  против заголовков выбранного релиза Dawn.
- Отдельно проверены C-адаптер GLFW–WebGPU и оба новых backend ImGui.
- Проверены размеры и смещения uniform-полей: `Uniforms` — 224 байта,
  `LightData` — 32 байта; шаг `VertexAttributes` — 44 байта.
- Численно проверена проекция камеры: near plane даёт NDC depth `0`, far — `1`.

В установленном MinGW не было `DXProgrammableCapture.h`, который нужен исходникам
Dawn даже при Vulkan backend. Для локальной проверки официальный заголовок
Microsoft добавлен в игнорируемую папку `build/modernization/sdk`, а путь передан
через `CPLUS_INCLUDE_PATH`. Вариант с MSVC предполагает установленный Windows SDK.
Для повторной сборки уже подготовленной локальной конфигурации в PowerShell:

```powershell
$env:CPLUS_INCLUDE_PATH = (Resolve-Path build/modernization/sdk).Path
cmake --build build/modernization/cmake --parallel
```

D3D12, Metal, Linux и браузерный запуск в этом окружении не проверялись.

## Что реализовывать дальше

Это последовательность практических задач, а не список уже добавленных функций.
Сначала доводи небольшой эксперимент до проверяемого результата, затем переходи
к следующему. Большую архитектурную перестройку имеет смысл делать, когда
появится конкретная потребность.

### 1. Математика, пространства и отладка

- [ ] В отдельном учебном модуле реализовать `Vec2/3/4`, `Mat3/4`, dot, cross,
  normalize, преобразования, look-at, perspective и orthographic. Сравнивать
  результаты с GLM до замены его в рендерере.
- [ ] Разобрать object/world/view/clip/NDC/screen spaces, homogeneous divide,
  handedness и порядок умножения матриц. Учесть глубину `[0, 1]`.
- [ ] Добавить визуализацию осей, нормалей, UV и глубины. Реализовать normal
  matrix как inverse transpose и проверить неравномерный scale.
- [ ] Для вращений изучить quaternion и slerp.

Результат: модель правильно преобразуется, а нормали и глубину можно увидеть
на экране и проверить численно.

### 2. Базовая растеризация и освещение

- [ ] Как отдельное небольшое упражнение написать CPU rasterizer: edge
  functions, barycentric coordinates, depth buffer, clipping и perspective-correct
  interpolation. Основной WebGPU-рендерер для этого переписывать не нужно.
- [ ] Сравнить Lambert, Phong и Blinn–Phong. Текущий `shader.wgsl` использует
  `reflect` и соответствует варианту Phong; для Blinn–Phong нужен half-vector.
- [ ] Добавить point/spot lights, затухание, несколько источников света
  и понятные границы параметров в UI.
- [ ] Разобраться с linear/sRGB и местом преобразования цвета. Проверить
  подключение `modelColor`: сейчас fragment shader использует цвет вершины.

Результат: переключаемые модели освещения на одной сцене без изменения камеры.

### 3. Текстуры и материалы

- [ ] Загружать albedo из `.mtl`, рисовать submesh с нужным material и bind group.
- [ ] Реализовать mipmaps и сравнить nearest, bilinear и trilinear filtering.
- [ ] Добавить normal mapping, расчёт tangent/bitangent и TBN basis.
- [ ] Отдельно обрабатывать цветовые и числовые текстуры: albedo обычно sRGB,
  normal/roughness/metallic — linear.

Результат: OBJ с несколькими материалами и normal map корректно отображается
при движении камеры и изменении масштаба модели.

### 4. Тени и несколько проходов

- [ ] Реализовать shadow map для directional light: depth pass из камеры света,
  затем сравнение глубины при освещении.
- [ ] Разобрать shadow acne, peter panning и bias; добавить PCF.
- [ ] После базового варианта сделать cascaded shadow maps. Тени point light
  через cubemap оставить следующим отдельным экспериментом.

Результат: видны depth texture и итоговая тень; понятно, почему появляются
артефакты и как меняются качество и стоимость фильтрации.

### 5. PBR и HDR

- [ ] Реализовать metallic/roughness PBR: Cook–Torrance, GGX distribution,
  Smith geometry, Schlick Fresnel и сохранение энергии.
- [ ] Рендерить освещение в HDR texture, добавить exposure и tone mapping.
- [ ] Добавить skybox, затем diffuse irradiance, specular prefilter и BRDF LUT
  для image-based lighting.

Результат: сетка сфер с разными roughness/metallic и предсказуемой реакцией
на одинаковое освещение.

### 6. Видимость и управление сценой

- [ ] Добавить список объектов с transform, mesh, material и bounds.
- [ ] Реализовать frustum culling по bounding sphere и AABB, отладочный вывод
  bounds и счётчики видимых объектов.
- [ ] Добавить instancing, сортировку opaque draw calls по pipeline/material,
  прозрачных — по глубине; изучить ограничения обычного alpha blending.
- [ ] Для picking реализовать ray–AABB и ray–triangle intersections, затем BVH.
- [ ] Позже сравнить forward и deferred rendering; для многих lights попробовать
  tiled или clustered lighting.

Результат: сравнение числа draw calls, времени CPU/GPU и числа проверок
пересечений до и после оптимизации на одинаковой сцене.

### 7. Compute и эффекты экрана

- [ ] Освоить storage buffers/textures, workgroups, shared memory, barriers,
  atomics и ping-pong buffers на системе частиц.
- [ ] Реализовать reduction, prefix sum и stream compaction; использовать их
  для отбора видимых объектов перед indirect draw.
- [ ] Сделать separable Gaussian blur и bloom, затем SSAO с blur. Текущий
  параметр `ambientOcclusion` задаёт постоянный ambient, геометрию он не учитывает.
- [ ] После этого попробовать FXAA, а затем TAA с motion vectors,
  history rejection и обработкой ghosting.

Результат: каждый эффект можно отключить, показать его промежуточную текстуру
и измерить отдельно от остальных проходов.

### 8. Трассировка как отдельный этап

- [ ] Написать CPU ray tracer: лучи камеры, сферы, треугольники и BVH.
- [ ] Добавить path tracing, Monte Carlo integration, PDF, cosine-weighted
  sampling, importance sampling, Russian roulette и накопление кадров.
- [ ] Перенести небольшой tracer в WGSL compute с собственным обходом BVH.
  Базовый WebGPU не предоставляет Vulkan/DXR ray tracing pipeline.

Результат: маленькая сцена с непрямым освещением; можно сравнить шум, время
и число samples per pixel с CPU-версией.

## Какие структуры данных знать и реализовать

Сначала используй STL в приложении. Собственные реализации полезно делать
как ограниченные упражнения и подключать после проверки корректности.

| Структура | Что понять и реализовать | Где применить |
| --- | --- | --- |
| Массив и dynamic array | capacity/size, рост, invalidation, contiguous memory | Вершины, индексы, объекты, команды отрисовки |
| Hash table | open addressing, collisions, load factor, rehash | Дедупликация OBJ-вершин, cache текстур и материалов |
| Stack, queue, deque | LIFO/FIFO, амортизированная сложность | Обход деревьев, очередь загрузки и задач |
| Binary heap | push/pop, priority queue | Приоритет загрузки ассетов, задачи по расстоянию |
| Arena и pool allocator | alignment, lifetime, free list | Временные данные кадра и объекты одинакового размера |
| Ring buffer | wrap-around, занятые диапазоны, завершение GPU work | Потоковая загрузка и dynamic uniforms без перезаписи используемых GPU данных |
| Handle с generation | index + generation, обнаружение устаревших ссылок | Ссылки на mesh/material после удаления и повторного использования слота |
| Sparse set | dense/sparse arrays, swap-remove, iteration | Компактное хранение компонентов при необходимости ECS |
| Дерево transforms | parent/children, DFS, dirty propagation | Иерархия сцены и пересчёт world matrices |
| AABB, sphere, plane, frustum | union, transform bounds, intersection | Culling, picking и coarse collision tests |
| BVH | median split, потом SAH, плоский массив узлов, обход стеком | Picking и ускорение ray/path tracing |
| Spatial hash / uniform grid | привязка координат к ячейке, соседние ячейки | Частицы и поиск соседей; octree/k-d tree изучать для сравнения |
| Граф и DAG | adjacency list, topological sort, cycle detection | Небольшой render graph после появления нескольких зависимых passes |
| Half-edge mesh | adjacency вершин/рёбер/граней, обход соседей | Позже: subdivision, упрощение и редактирование mesh |

Для каждой реализации проверь пустой контейнер, граничные размеры, удаление,
повторное использование и сложность операций. Для BVH сравни результат
пересечений с полным перебором треугольников. Для математики используй численные
проверки с допуском, включая вырожденные случаи.

Отдельно изучи AoS/SoA, cache locality, ownership/RAII, move semantics,
выравнивание CPU/WGSL-структур, lifetime ресурсов GPU, асинхронный readback
и разницу между временем отправки команды CPU и её выполнения GPU.

## Как фиксировать результат каждого алгоритма

Сохраняй небольшую тестовую сцену, описание формул, изображения промежуточных
буферов, известные артефакты и измерение времени в миллисекундах. Меняй один
эффект за раз. Для GPU timestamps сначала проверяй поддержку нужной feature;
одного FPS или CPU-таймера вокруг `queue.Submit` недостаточно.

Хороший первый маршрут для этого проекта:
**нормали и пространства → текстуры → normal mapping → shadow mapping → PBR →
culling/BVH → compute → post-processing → path tracing**.

## Источники

- [Выбранный релиз Dawn](https://github.com/google/dawn/releases/tag/v20261016.190333)
  и [официальная сборка Dawn с CMake](https://github.com/google/dawn/blob/v20261016.190333/docs/quickstart-cmake.md).
- [Backend WebGPU ImGui v1.93.0](https://github.com/ocornut/imgui/blob/v1.93.0/backends/imgui_impl_wgpu.h)
  — новый init API и требования к Dawn/Emscripten.
- [Спецификация WebGPU](https://www.w3.org/TR/webgpu/) и
  [спецификация WGSL](https://www.w3.org/TR/WGSL/) — ресурсы, passes, типы и layout.
- [Learn WebGPU for C++](https://eliemichel.github.io/LearnWebGPU/) — практика
  работы с WebGPU; версию API в примерах нужно сверять с закреплённым Dawn.
- [Physically Based Rendering, 4-е издание](https://pbr-book.org/4ed/contents)
  — геометрия, BVH, sampling, материалы и path tracing.
