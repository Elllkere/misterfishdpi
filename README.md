# MisterFish DPI
Обход блокировок РКН через [zapret](https://github.com/bol-van/zapret/) Windows

> [!IMPORTANT]
> Я не являюсь разработчиком дурения DPI, я лишь разработал интерфейс для удобного управления оригинального zapret (с добавлеием пресетов настроек)

> [!WARNING]
> Возможна неправильная работа с VPN приложениеями, включая VPNGame, Exitlag и прочих снижающих пинг приложений.
> Эта проблема не относится конкретно к моему приложение, а относится напрямую к работе zapret.

## Как запустить?
> [!WARNING]
> Перед запуском приложения нужно удалить предыдущие способы обхода (zapret, GoodbyeDPI и прочие).

> [!IMPORTANT]
> При наличии антивирусов \ защитника windows \ smartscreen нужно добавить в исключение либо всю папку, либо MisterFish.exe и winws.exe

> [!TIP]
> Присутствуют два тихих режима, которые выключают автоматическую проверку версий и не создают окно приложения. Нужно передать один из них в аргумены запуска приложения
> 
> -silent
> 
> -verysilent - не создаст ничего, ни окна, ни трея

> [!TIP]
> -console включит консольный режим без возможности управлять включением и выключением сервисов (только через интерфейс или конфиг)

Скачать архив из последнего [релиза](https://github.com/Elllkere/misterfishdpi/releases/latest), разархивировать в любую пустую папку и запустить MisterFish.exe от имени администратора.
Кнопка `_` (свернуть) свернет приложение в трей, кнопка `х` (закрыть) закроет полностью приложение, включая все ранее включенные обходы сервисов.

[Использование с VPNGame](https://github.com/Elllkere/misterfishdpi/discussions/3)

> [!IMPORTANT]
> В настройках приложения важно выбрать своего интернет-провайдера, иначе обходы сервисов могут не работать, если провайдер выбран правильно и сервис запущен без ошибок, но сервис все равно не доступен, тогда стоит создать Issue.

> [!TIP]
> Приложение поддерживает автозапуск в двух режимах.
>
> Первый режим через планировщик задач, подходит для всех, особенно для пользователей, у которых не отключен UAC (контроль учетных записей) и при запуске любого файла появляется окно с подтверждением. С этим режимом у таких пользователей приложение будет запускаться без окна подтверждения при запуске Windows.
>
> Второй режим через реестр, подходит только для пользователей с выключенным UAC, т.к. при запуске Windows приложение либо не запуститься, либо появится окно с подтверждением запуска.

## Зачем?
Многим людям лень или сложно разбираться в настройках дурения DPI, к тому же следить за всем этим еще и запускать через какие то батники \ сервисы. Я решил разработать удобный интерфейс управления с моими подготовленными пресетами для заблокированных сервисов, дабы люди заходили, нажимали одну кнопку и не думали более ни о чем.

> [!TIP]
> Я буду стараться следить за актуальностью пресетов, но буду благодарен, если кто - то будет присылать новые пресеты для новых блокировок или исправления, а так же других пресетов для определенных провайдеров.
>
> Присылать пресеты можно в дискуссии, либо напрямую в pull requests, меняя [Zapret::getArgs()](https://github.com/Elllkere/misterfishdpi/blob/main/ZapretGUI/zapret/zapret_imp.hpp#L213).
>
> Проверять работоспособность можно на оригинальном [zapret](https://github.com/bol-van/zapret/), только не забывать, что это только для Windows, соотвественно проверять там же через winws (nfqws) или можно напрямую из моей релизной версии winws.

## Почему MisterFish?
https://www.youtube.com/watch?v=I7JVD-wo3cI

## Credits & Contributors
* [bol-van](https://github.com/bol-van/), creator of original [zapret](https://github.com/bol-van/zapret/) repository.
* [ocornut](https://github.com/ocornut/), creator of original [ImGui](https://github.com/ocornut/imgui) repository.
