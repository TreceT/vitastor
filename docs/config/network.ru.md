[Документация](../../README-ru.md#документация) → [Конфигурация](../config.ru.md) → Параметры сетевого протокола

-----

[Read in English](network.en.md)

# Параметры сетевого протокола

Данные параметры используются клиентами и OSD и влияют на логику сетевого
взаимодействия между клиентами, OSD, а также etcd.

- [tcp_header_buffer_size](#tcp_header_buffer_size)
- [use_sync_send_recv](#use_sync_send_recv)
- [use_rdma](#use_rdma)
- [rdma_device](#rdma_device)
- [rdma_port_num](#rdma_port_num)
- [rdma_gid_index](#rdma_gid_index)
- [rdma_mtu](#rdma_mtu)
- [rdma_max_sge](#rdma_max_sge)
- [rdma_max_msg](#rdma_max_msg)
- [rdma_max_recv](#rdma_max_recv)
- [peer_connect_interval](#peer_connect_interval)
- [peer_connect_timeout](#peer_connect_timeout)
- [osd_idle_timeout](#osd_idle_timeout)
- [osd_ping_timeout](#osd_ping_timeout)
- [up_wait_retry_interval](#up_wait_retry_interval)
- [max_etcd_attempts](#max_etcd_attempts)
- [etcd_quick_timeout](#etcd_quick_timeout)
- [etcd_slow_timeout](#etcd_slow_timeout)
- [etcd_keepalive_timeout](#etcd_keepalive_timeout)
- [etcd_ws_keepalive_timeout](#etcd_ws_keepalive_timeout)
- [client_dirty_limit](#client_dirty_limit)

## tcp_header_buffer_size

- Тип: целое число
- Значение по умолчанию: 65536

Размер буфера для чтения данных с дополнительным копированием. Пакеты
Vitastor содержат 128-байтные заголовки, за которыми следуют данные размером
от 4 КБ и для мелких операций ввода-вывода обычно выгодно за 1 вызов читать
сразу несколько пакетов, даже не смотря на то, что это требует лишний раз
скопировать данные. Часть каждого пакета за пределами значения данного
параметра читается без дополнительного копирования. Вы можете попробовать
поменять этот параметр и посмотреть, как он влияет на производительность
случайного и линейного доступа.

## use_sync_send_recv

- Тип: булево (да/нет)
- Значение по умолчанию: false

Если установлено в истину, то вместо io_uring для передачи данных по сети
будут использоваться обычные синхронные системные вызовы send/recv. Для OSD
это бессмысленно, так как OSD в любом случае нуждается в io_uring, но, в
принципе, это может применяться для клиентов со старыми версиями ядра.

## use_rdma

- Тип: булево (да/нет)
- Значение по умолчанию: true

Пытаться использовать RDMA для связи при наличии доступных устройств.
Отключите, если вы не хотите, чтобы Vitastor использовал RDMA.
TCP-клиенты также могут работать с RDMA-кластером, так что отключать
RDMA может быть нужно только если у клиентов есть RDMA-устройства,
но они не имеют соединения с кластером Vitastor.

## rdma_device

- Тип: строка

Название RDMA-устройства для связи с Vitastor OSD (например, "rocep5s0f0").
Имейте в виду, что поддержка RDMA в Vitastor требует функций устройства
Implicit On-Demand Paging (Implicit ODP) и Scatter/Gather (SG). Например,
адаптеры Mellanox ConnectX-3 и более старые не поддерживают Implicit ODP и
потому не поддерживаются в Vitastor. Запустите `ibv_devinfo -v` от имени
суперпользователя, чтобы посмотреть список доступных RDMA-устройств, их
параметры и возможности.

## rdma_port_num

- Тип: целое число
- Значение по умолчанию: 1

Номер порта RDMA-устройства, который следует использовать. Имеет смысл
только для устройств, у которых более 1 порта. Чтобы узнать, сколько портов
у вашего адаптера, посмотрите `phys_port_cnt` в выводе команды
`ibv_devinfo -v`.

## rdma_gid_index

- Тип: целое число
- Значение по умолчанию: 0

Номер глобального идентификатора адреса RDMA-устройства, который следует
использовать. Разным gid_index могут соответствовать разные протоколы связи:
RoCEv1, RoCEv2, iWARP. Чтобы понять, какой нужен вам - смотрите строчки со
словом "GID" в выводе команды `ibv_devinfo -v`.

**ВАЖНО:** Если вы хотите использовать RoCEv2 (как мы и рекомендуем), то
правильный rdma_gid_index, как правило, 1 (IPv6) или 3 (IPv4).

## rdma_mtu

- Тип: целое число
- Значение по умолчанию: 4096

Максимальная единица передачи (Path MTU) для RDMA. Должно быть равно 1024,
2048 или 4096. Обычно нет смысла менять значение по умолчанию, равное 4096.

## rdma_max_sge

- Тип: целое число
- Значение по умолчанию: 128

Максимальное число записей разделения/сборки (scatter/gather) для RDMA.
OSD в любом случае согласовывают реальное значение при установке соединения,
так что менять этот параметр обычно не нужно.

## rdma_max_msg

- Тип: целое число
- Значение по умолчанию: 1048576

Максимальный размер одной RDMA-операции отправки или приёма.

## rdma_max_recv

- Тип: целое число
- Значение по умолчанию: 8

Максимальное число параллельных RDMA-операций получения данных. Следует
иметь в виду, что данное число буферов размером `rdma_max_msg` выделяется
для каждого подключённого клиентского соединения, так что данная настройка
влияет на потребление памяти. Это так потому, что RDMA-приём данных в
Vitastor, увы, всё равно не является zero-copy, т.е. всё равно 1 раз
копирует данные в памяти. Данная особенность, возможно, будет исправлена в
более новых версиях Vitastor.

## peer_connect_interval

- Тип: секунды
- Значение по умолчанию: 5
- Минимальное значение: 1

Время ожидания перед повторной попыткой соединиться с недоступным OSD.

## peer_connect_timeout

- Тип: секунды
- Значение по умолчанию: 5
- Минимальное значение: 1

Максимальное время ожидания попытки соединения с OSD.

## osd_idle_timeout

- Тип: секунды
- Значение по умолчанию: 5
- Минимальное значение: 1

Время неактивности соединения с OSD, после которого клиенты или другие OSD
посылают запрос проверки состояния соединения.

## osd_ping_timeout

- Тип: секунды
- Значение по умолчанию: 5
- Минимальное значение: 1

Максимальное время ожидания ответа на запрос проверки состояния соединения.
Если OSD не отвечает за это время, соединение отключается и производится
повторная попытка соединения.

## up_wait_retry_interval

- Тип: миллисекунды
- Значение по умолчанию: 500
- Минимальное значение: 50

Когда OSD получают от клиентов запросы ввода-вывода, относящиеся к не
поднятым на данный момент на них PG, либо к PG в процессе синхронизации,
они отвечают клиентам специальным кодом ошибки, означающим, что клиент
должен некоторое время подождать перед повторением запроса. Именно это время
ожидания задаёт данный параметр.

## max_etcd_attempts

- Тип: целое число
- Значение по умолчанию: 5

Максимальное число попыток выполнения запросов к etcd для тех запросов,
которые нельзя повторять бесконечно.

## etcd_quick_timeout

- Тип: миллисекунды
- Значение по умолчанию: 1000

Максимальное время выполнения запросов к etcd, которые должны завершаться
быстро, таких, как обновление резервации (lease).

## etcd_slow_timeout

- Тип: миллисекунды
- Значение по умолчанию: 5000

Максимальное время выполнения запросов к etcd, для которых не обязательно
гарантировать быстрое выполнение.

## etcd_keepalive_timeout

- Тип: секунды
- Значение по умолчанию: max(30, etcd_report_interval*2)

Таймаут для HTTP Keep-Alive в соединениях к etcd. Должен быть больше, чем
etcd_report_interval, чтобы keepalive гарантированно работал.

## etcd_ws_keepalive_timeout

- Тип: секунды
- Значение по умолчанию: 30

Интервал проверки живости вебсокет-подключений к etcd.

## client_dirty_limit

- Тип: целое число
- Значение по умолчанию: 33554432

При работе без immediate_commit=all - это лимит объёма "грязных" (не
зафиксированных fsync-ом) данных, при достижении которого клиент будет
принудительно вызывать fsync и фиксировать данные. Также стоит иметь в виду,
что в этом случае до момента fsync клиент хранит копию незафиксированных
данных в памяти, то есть, настройка влияет на потребление памяти клиентами.

Параметр не влияет на сами OSD.
