# Лабораторная работа 1

**Название:** "Разработка драйверов символьных устройств"

**Цель работы:** ...

## Описание функциональности драйвера
При записи текста в файл символьного устройства должен осуществляться подсчет введенных символов. Последовательность полученных результатов (количество символов) с момента загрузки модуля ядра должна выводиться при чтении созданного файла /proc/varN в консоль пользователя.

При чтении из файла символьного устройства в кольцевой буфер ядра должен осуществляться вывод тех же данных, которые выводятся при чтении файла /proc/varN.
...

## Инструкция по сборке

make

sudo insmod lab1.ko

sudo chmod a+w /dev/var1

...

## Инструкция пользователя
Записать в файл /dev/var1

Считать из файла /proc/var1
...

## Примеры использования
echo "random text" > /dev/var1

cat /proc/var1

echo "skl" > /dev/var1

cat /proc/var1

...
