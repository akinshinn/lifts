#include <cmath>
#include <iostream>
#include <list>
#include <map>

#include "Control.h"

/*! \struct myParams
 \brief Структура, содержащая пользовательские параметры

 Данные параметры сохряняются при переходе от одного шага по времени к следующим,
 их можно использовать для запоминания необходимых параметров.
 Набор членов-данных структуры можно модифицировать по собственному усмотрению.
*/
//Задание условия задачи

/// Число лифтов
const size_t numberOfElevators = 2;

/// Вместимость лифта
/// \warning Тренироваться проще с меньшей вместимостью, в реальной задаче будет не менее 6 человек
const size_t elevatorCapacity = 8;

/// Максимальный номер этажа (не считая подвала, который имеет номер 0).
/// Пассажиры иногда ездят в подвал и из подвала
const size_t maxFloor = 11;

/// Общее число этажей
const size_t numberOfFloors = maxFloor + 1;

/// Время моделирования в секундах
/// \warning Cейчас для тестирования задано 26000 секунд, в реальной задаче может быть, скажем, 54000 секунд: от 7:00 утра до 22:00 вечера
const size_t maxTime = 900;
struct myParams
{
    /// Некоторый произвольный параметр, инициализированный значением "0"
    size_t arbitraryParam = 0;

    /// Признак того, что лифт выполняет работу
    bool started = false;
    //можно добавить любое число параметров любого типа

    size_t find_current_destination(Control& control, int elv) {
        double pos = control.getElevatorPosition(elv);
        int min = 1000;
        int res = 0;
        std::vector d_btns = control.getFloorDnButtons();
        std::vector u_btns = control.getFloorUpButtons();
        std::vector elv_btns = control.getElevatorButtons(elv);
        for (int i = 0; i < numberOfFloors; i++) {
            if ((d_btns[i] || u_btns[i] || elv_btns[i])) {
                double dist = abs(pos - i);
                if (dist < min) {
                    min = dist;
                    res = i;
                }
            }
        }
        return res;
    };

    size_t find_current_destination_elv_btn(Control& control, int elv) {
        double pos = control.getElevatorPosition(elv);
        int min = 1000;
        int res = 0;
        std::vector elv_btns = control.getElevatorButtons(elv);
        for (int i = 0; i < numberOfFloors; i++) {
            if (elv_btns[i]) {
                double dist = abs(pos - i);
                if (dist < min) {
                    min = dist;
                    res = i;
                }
            }
        }
        return res;
    };



    bool is_elevator_full(Control& control, size_t elev, size_t floor) {
        static int counter = 0;
        if (control.getElevatorButton(elev, floor)) counter--;
        if (control.getFloorDnButton(floor) + control.getFloorUpButton(floor)) counter++;

        return (counter >= elevatorCapacity);
    }
    size_t find_upper_dest(Control& control, size_t elv, size_t floor) {
        std::vector<bool> u_btns = control.getFloorUpButtons();
        std::vector<bool> elv_btns = control.getElevatorButtons(elv);
        for (int i = floor+1; i < numberOfFloors; i++) {
            if (u_btns[i] || elv_btns[i]) return i;
        }
        return floor;
    }
    size_t find_dnwn_dest(Control& control, size_t elv, size_t floor) {
        std::vector<bool> d_btns = control.getFloorDnButtons();
        std::vector<bool> elv_btns = control.getElevatorButtons(elv); std::cout << "in dwn dest\n";
        if (control.getCurrentTime() == 138) std::cout << "CTIME = 138 " << elv_btns[1] << "\n";
        for (int i = floor-1; i >= 0; i--) {
            if (d_btns[i] || elv_btns[i]) return i;
        }
        return floor;
    }
};



/// \brief Основная функция системы управления
/// 
/// Именно эту функцию нужно модифицировав, реализовав здесь оптимизированный
/// алгорим работы пассажирского лифта.
/// 
/// Другие функции "трогать" запрещается.
/// 
/// Данная функция вызывается на каждом шаге (каждую секунду) работы лифта.
/// 
/// Внутри можно пользоваться структурой params, сохраняя в нее при необходимости нужные сведения,
/// которые, соответственно, будут доступны при следующем вызове функции CONTROLSYSTEM
/// 
/// Cобственно, для активного управления лифтами есть всего две команды:
///  - control.setElevatorDestination(elevatorNumber, newDestination);
///  - control.setElevatorIndicator(elevatorNumber, newIndicator)
/// 
/// Прежде, чем отдавать команду лифту по итогам анализа его текущего состояния - надо решить, 
/// имеет ли смысл делать это прямо сейчас
/// 
/// К примеру, если лифт едет куда-то на вызов (скажем, на 10-й этаж), и в этот момент кто-то
/// в подвале (0-й этаж) нажал на кнопку вызова, 
/// то если поступить формально и в этот момент изменить назначение лифта путем исполнения команды
///  
/// - control.setElevatorDestination(elev, newDestination), 
///
/// то он затормозит по пути (возможно, даже между этажами), и потом начнет разгоняться 
/// и поедет в обратном направлении (вниз);
/// если лифт прибыл на этаж, начал тормозить, и в этот момент ему установить новое назначение,
/// то он затормозит, двери не откроет, и тут же поедет по новому назначению!
///   
/// Будьте аккуратны. Наш лифт очень "исполнительный"!
///
/// Поэтому нужно тщательно обдумывать, в какой момент отдавать команду на изменение направления
/// и в какой команд отдавать команду на изменение индикатора
///
///  - control.setElevatorIndicator(elev, newIndicator)
/// 
/// (пассажиры садятся только в тот лифт, который едет в нужную им сторону, 
///  судя по индикатору, который они видят, хотя иногда попадаются и те, кто садятся не туда,
///  при этом зайдя в лифт, они "жмут" кнопку, куда надо им --- это тоже надо как-то обрабатывать!!!) 
/// 
/// \param[in,out] control ссылка на основной класс-симулятор лифта
/// \param[in,out] params ссылка на набор пользовательских параметров
void CONTROLSYSTEM(Control& control, myParams& params);


int main(int argc, char** argv)
{
    //Задание конфигурации лифтового хозяйства
    Control control(numberOfFloors, numberOfElevators, elevatorCapacity);
    //Для загрузки расписания появления пассажиров из файла
    //control.ReadTimeTable("TimeTable/timetable500a.csv");
    //std::cout << "main";

    //Для тестирования вместо чтения из файла (строка выше)
    //можно вводить появляющихся пассажиров вручную    
    //параметры в фиг. скобках
    //{
    // 1) время появления пассажира (от начала моделирования)
    // 2) этаж, где появляется пассажир
    // 3) этаж, куда направляется пассажир
    // 4) время, которое пассажир ждет и после которого, не вырерживая, уходит (начисляется штраф)
    // 5) вероятность сесть в лифт, идущий в обратном направлении, в начале ожидания
    // 6) вероятность сесть в лифт, идущий в обратном направлении, в конце ожидания
    // 7) вероятеность того, что пассажир, войдя в лифт, нажмет "ход" и лифт не будет стоять
    // }
    /*
    control.AddPassengerToQueue({  5, 5,  3, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({  6, 5, 10, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({  7, 5,  2, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({  8, 5,  8, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({  9, 5, 10, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({ 10, 5,  6, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({ 11, 5,  9, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({ 12, 5,  8, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({ 13, 5, 11, 300, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({ 14, 5, 10, 300, 0.01, 0.20, 0.50 });
    */
    control.AddPassengerToQueue({ 1, 1, 5, 600, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({ 4, 2, 6, 600, 0.01, 0.20, 0.50 });
    control.AddPassengerToQueue({ 1, 3, 1, 600, 0.01, 0.20, 0.50 });

    
    myParams params;
    do
    {
        //Выполнение одного шага (= 1 секунда) моделирования работы лифта
        control.MakeStep();
        //Вызов функции системы управления --- в ней можно "отдать команду" лифту,
        //исходя из его текущего состояния и состояния кнопок в лифте и на этажах
        CONTROLSYSTEM(control, params);

        //Вывод состояния лифта
        //control.PrintElevatorState(0);                //Вывод состояния лифта #0 на экран
        //control.PrintElevatorState(1);                //Вывод состояния лифта #1 на экран

        control.PrintElevatorState(0, "fileElev0.txt"); //Вывод состояния лифта #0 в файл
        control.PrintElevatorState(1, "fileElev1.txt"); //Вывод состояния лифта #1 в файл

        //Вывод состояния кнопок в лифте и на этажах
        //control.PrintButtonsState();                  //Вывод состояния кнопок на экран
        control.PrintButtonsState("fileButtons.txt");   //Вывод состояния кнопок в файл

        //Вывод событий появлений пассажиров, их входа в лифт, выхода из лифта, ухода с этажа
        //control.PrintPassengerState();                    //Вывод статистики пассажиров на экран
        control.PrintPassengerState("filePassengers.txt");  //Вывод статистики пассажиров в файл

    } while (control.getCurrentTime() <= maxTime);

    //Печать итоговой статистики в конце работы симулятора   
    control.PrintStatistics(true, "Statistics.txt");

    return 0;
}


// Здесь пример системы управления --- именно эту функцию надо модифицировать.
// Другие функции "трогать" запрещается
// Данная функция вызывается на каждом шаге (каждую секунду) работы лифта
//   
// Внутри можно пользоваться структурой params, сохраняя в нее при необходимости нужные сведения,
// которые, соответственно, будут доступны при следующем вызове функции CONTROLSYSTEM
void CONTROLSYSTEM(Control& control, myParams& params)
{
    // Прежде, чем отдавать команду лифту по итогам анализа его текущего состояния - надо решить, 
    // имеет ли смысл делать это прямо сейчас
    // 
    // К примеру, если лифт едет куда-то на вызов (скажем, на 10-й этаж), и в этот момент кто-то
    // в подвале (0-й этаж) нажал на кнопку вызова, 
    // то если поступить формально и в этот момент изменить назначение лифта путем исполнения команды
    // 
    //  control.setElevatorDestination(elev, newDestination), 
    // 
    // то он затормозит по пути (возможно, даже между этажами), и потом начнет разгоняться 
    // и поедет в обратном направлении (вниз);
    // если лифт прибыл на этаж, начал тормозить, и в этот момент ему установить новое назначение,
    // то он затормозит, двери не откроет, и тут же поедет по новому назначению!
    // Будьте аккуратны. Наш лифт очень "исполнительный"!
    //      
    // Поэтому нужно тщательно обдумывать, в какой момент отдавать команду на изменение направления
    // и в какой команд отдавать команду на изменение индикатора
    // 
    //  control.setElevatorIndicator(elev, newIndicator)
    // 
    //  (пассажиры садятся только в тот лифт, который едет в нужную им сторону, 
    //   судя по индикатору, который они видят, хотя иногда попадаются и те, кто садятся не туда,
    //   при этом зайдя в лифт, они "жмут" кнопку, куда надо им --- это тоже надо как-то обрабатывать!!!) 
    // 
    //  ПРИМЕЧАНИЕ: собственно, для активного управления лифтами есть всего две команды:
    //   control.setElevatorDestination(elev, newDestination);
    //   control.setElevatorIndicator(elev, newIndicator)
    // 


    // Для получения текущего времени можно пользоваться командой
    //  control.getCurrentTime()


    // Следующие команды носят характер опроса текущего состояния лифта    
    // 
    // - текущее назначение лифта:
    //   control.getElevatorDestination(elev); 
    //
    // - текущее начение индикатора:
    //   control.getElevatorIndicator(elev);
    // 
    // - текущее положение лифта (дробное число, когда лифт между этажами;
    //     даже если оно целое - лифт не обязательно с открытыми дверьми, он может начинать разгоняться 
    //     или тормозить и т.п.)
    //   control.getElevatorPosition(elev);
    //
    // - признак того, что лифт движется вверх
    //   control.isElevatorGoingUp(elev)
    //
    // - признак того, что лифт движется вниз
    //   control.isElevatorGoingDn(elev)
    //
    // - признак того, что лифт стоит на месте
    //   control.isElevatorStaying(elev)
    //  
    // - признак того, что лифт движется равномерно 
    //   control.isElevatorGoingUniformly(elev);
    //
    // - признак того, что лифт движется с ускорением (разгоняется)
    //   control.isElevatorAccelerating(elev);
    //
    // - признак того, что лифт движется с замедлением (тормозит)
    //   control.isElevatorBreaking(elev);
    //
    // - признак того, что лифт стоит на месте (на этаже) с закрытыми дверьми 
    //   control.isElevatorStayingDoorsClosed(elev);
    //
    // - признак того, что в текущий сомент происходит закрывание дверей
    //   control.isElevatorDoorsClosing(elev);
    //
    // - признак того, что в текущий сомент происходит открывание дверей
    //   control.isElevatorDoorsOpening(elev);
    //
    // - признак того, что в текущий момент двери открыты
    //   control.isElevatorDoorsOpened(elev);
    //
    // - признак того, что лифт пустой (в нем нет ни одного пассажира)
    //   control.isElevatorEmpty(elev);
    //
    // - признак того, что в данный момент завершилась высадка пассажиров, и лифт оказался пустым (см. ниже)
    //   control.isElevatorEmptyAfterUnloading(elev);
    //
    // - признак того, что лифт достиг точки назначения
    //    точка назначения считается достигнутой, когда
    //     1) лифт приехал на тот этаж, куда его послали, остановился, и
    //     2) выполнено одно из трех условий:
    //        а) в нем есть хотя бы 1 пассажир - тогда открылись двери
    //        б) он пустой, а на этаже, на который он прибыл, нажата хотя бы одна 
    //           кнопка - тогда тоже открылись двери
    //        в) он пустой, а на этаже, на который он прибыл, не нажато ни одной 
    //           кнопки - тогда двери не открываются  
    //   control.isElevatorAchievedDestination(elev)
    //
    //
    // Может быть полезной команда
    // 
    //  control.isElevatorEmptyAfterUnloading(elev)
    // 
    // которая возвращает true, если лифт стоит на этаже, и после выхода очередного пассажира 
    // лифт оказался пустым --- возможно, при этом имеет смысл "включить" индикатор в оба направления, 
    // чтобы в любом случае зашел пассажир, стоящий первым в очереди.
    // Но это не обязательно - у Вас может быть своя логика!
    // 
    // Если индикатор лифта "горит" в состоянии both (он пустой или нет - не важно), 
    // и в лифт входит пассажир, то индикатор автоматически переключается в то направление,
    // какую кнопку он нажал, входя в лифт.
    // Будьте осторожны, "зажигайте" состояние индикатора both аккуратно, но и без него обойтись будет трудно!
    // 

    // Следующие 4 команды позволяют узнать состояние "нажатости" кнопок на этажах
    // 	const std::vector<bool>& getFloorUpButtons() const
    //
    // При этом когда лифт приезжает на какой-либо этаж, то в момент открывания дверей на этаже
    //  автоматически гаснет та кнопка, какой индикатор в этот момент установлен у лифта
    //  (если индикатор both - гаснут обе кнопки)
    // Если пассажиры, оставшиеся на этаже, видят, что нужная им кнопка погасла, они 
    //  нажмут ее снова, как только лифт тронется
    //
    // - возвращает вектор (массив) состояний нажатия кнопок вверх
    //   control.floorButtons->getUpButtons();
    //
    // - возвращает вектор (массив) состояний нажатия кнопок вниз
    //   control.floorButtons->getUpButtons();
    //
    // - возвращает состояние нажатия кнопки вверх на i-м этаже
    //   control.floorButtons->getUpButton(i);
    //
    // - возвращает состояние нажатия кнопки вниз на i-м этаже
    //   control.floorButtons->getDnButton(i);
    //
    // При необходимости можно использовать команды принудительного выключения кнопок на соответствующих этажах:
    //  control.unsetUpButton(floor);
    //  control.unsetDnButton(floor);


    // Наконец, еще 2 команды позволяют оценить состояние кнопок в кабине лифта
    // Человек, входящий в лифт, нажимает кнопку этажа назначения
    // Кнопка, нажатая внутри лифта, гаснет, когда лифт прибывает на этаж и начинает открывать двери
    //
    // - возвращает вектор состояния нажатости кнопок в кабине лифта
    //   control.getElevatorButtons(elev)
    // 
    // - возвращает вектор состояния нажатости кнопки i-го этажа в кабине лифта
    //   control.getElevatorButton(elev, i)


    //////////////////////////////////////////////////////////////////////////////////
    // ПРИМЕР примитивной системы управления, при которой первоначально лифт #0 стоит
    // в подвале, а лифту #1 отдается команда уехать на самый верхний этаж.
    // Потом они оба ждут до момента появления первого пассажира на каком-либо этаже, 
    // после чего начинают кататься вверх-вниз, останавливаясь на каждом этаже 
    // т.е. вообще не реагируя на кнопки!
    //////////////////////////////////////////////////////////////////////////////////

    if (control.getCurrentTime() == 1)
    {
        control.SetElevatorDestination(1, maxFloor);
        control.SetElevatorIndicator(1, ElevatorIndicator::up); // поменял на both(up)
    }

    if (!params.started)
    {
        size_t nUp = std::count(control.getFloorUpButtons().begin(), control.getFloorUpButtons().end(), true);
        size_t nDn = std::count(control.getFloorDnButtons().begin(), control.getFloorDnButtons().end(), true);

        //Если хоть одна кнопка вверх или вниз на этажах нажата - запускаем лифт!
        if (nUp + nDn > 0)
        {
            params.started = true;
        }
    }
    // size_t global_dest = 5;
    for (size_t elv = 0; elv < 1; ++elv)
    {
        // В данном примере новая команда (назначение) не отдается,
        // пока не выполнена предыдущая
        if ((params.started) && (control.isElevatorAchievedDestination(elv)))
        {
            // считываем этаж, на который лифт прибыл

            //control.SetElevatorIndicator(elv, ElevatorIndicator::both);
            // прибывая на этаж назначения лифт открывает двери, если либо он непустой,
            // либо на этом этаже нажата кнопка вызова хотя бы в какую-то сторону,
            // в противном случае прибывает на этаж и стоит, не открывая двери
            // считываем текущее положение лифта
            size_t pos = (size_t)(control.getElevatorPosition(elv));
            control.unsetDnButton(pos);
            control.unsetUpButton(pos);
            size_t global_dest;
            if (!(params.is_elevator_full(control, elv, pos))) {
                if (control.getElevatorIndicator(elv) == ElevatorIndicator::up || control.getElevatorIndicator(elv) == ElevatorIndicator::both) {
                    global_dest = params.find_upper_dest(control, elv, pos);
                    if (global_dest == pos && params.find_dnwn_dest(control, elv, pos) != pos ) control.SetElevatorIndicator(elv, ElevatorIndicator::down);
                }
                else {
                    global_dest = params.find_dnwn_dest(control, elv, pos);
                    if (global_dest == pos && params.find_upper_dest(control, elv, pos) != pos) control.SetElevatorIndicator(elv, ElevatorIndicator::up);
                }
            }
            else {
                global_dest = params.find_current_destination_elv_btn(control, elv);
                if (global_dest > pos) control.SetElevatorIndicator(elv, ElevatorIndicator::up);
                else if (global_dest == pos) control.SetElevatorIndicator(elv, ElevatorIndicator::both);
                else  control.SetElevatorIndicator(elv, ElevatorIndicator::down);

            }
            //if (params.is_elevator_full(control, elv, nextDest))
            //    global_dest = params.find_current_destination_elv_btn(control, elv);
            //else global_dest = params.find_current_destination(control, elv);

            //std::cout << params.find_current_destination(control, elv) << " " << control.getCurrentTime() << " " << control.getElevatorPosition(elv) << std::endl;

            //if (global_dest > nextDest) control.SetElevatorIndicator(elv, ElevatorIndicator::up);

            //else control.SetElevatorIndicator(elv, ElevatorIndicator::down);
            std::cout << "time = " << control.getCurrentTime() << " dest = " << global_dest << "\n";

            control.SetElevatorDestination(elv, global_dest);
        }


    }

    /*
     if (control.getCurrentTime() < 5)
         control.SetElevatorDestination(1, 2);
     else
         control.SetElevatorDestination(1, 0);
    */
}