#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include "Control.h"



#include <fstream>


const size_t numberOfElevators = 2;

const size_t elevatorCapacity = 8;

const size_t maxFloor = 11;

const size_t numberOfFloors = maxFloor + 1;

const size_t maxTime = 6000;
int m = 10, n = 10, k = 210, f = 260;
struct myParams
{
    Control* control = nullptr; // указатель на класс Control для удобства
    bool started = false; // запущены ли лифты
    size_t* time_exceed_btns_per_floors = new size_t[numberOfFloors]; // Время ожидания на этаже снаружи, т.е. нажата кнопка вне кабины
    bool* is_elvs_fulled = new bool[numberOfElevators]; // Заполнен ли i-й лифт
    size_t** time_exceed_floors_in_elv = new size_t*[numberOfElevators]; // Время ожидания на этаже для кабины лифта


    // Инициализация вспомогательных данных
    void init() {
        for (int floor = 0; floor < numberOfFloors; floor++) {
            time_exceed_btns_per_floors[floor] = 0;
        }
        for (int elv = 0; elv < numberOfElevators; elv++) {
            is_elvs_fulled[elv] = 0;
            time_exceed_floors_in_elv[elv] = new size_t[numberOfFloors];
        }
    };


    // Деструктор для удаления всех вспомогательных массивов
    ~myParams() {
        delete[] time_exceed_btns_per_floors, is_elvs_fulled; 
        for (int i = 0; i < numberOfElevators; i++) {
            delete[] time_exceed_floors_in_elv[i];
        }
        delete[] time_exceed_floors_in_elv;
    }


    // Функция для получения информации о том, находится ли этаж в направлении движения лифта
    bool get_indicator(size_t floor, size_t elv) {
        if (control->getElevatorIndicator(elv) == ElevatorIndicator::up && floor > control->getElevatorPosition(elv)) return true;
        else if (control->getElevatorIndicator(elv) == ElevatorIndicator::down && floor < control->getElevatorPosition(elv)) return true;
        else if (control->getElevatorIndicator(elv) == ElevatorIndicator::both) return true;
        return false;
    }
    // Увеличение времени ожидания на этаже ежесекундно 
    void time_exceed_increment(size_t elv) {
        for (size_t floor = 0; floor < numberOfFloors; floor++) {
            time_exceed_btns_per_floors[floor] +=  control->getFloorDnButton(floor) + control->getFloorUpButton(floor);
            time_exceed_floors_in_elv[elv][floor] += control->getElevatorButton(elv, floor);
        }

    }

    // Функция для подсчета приоритета для этажа
    double calc_priority_floor(size_t floor, size_t elv) {
        int is_btns_pressed = control->getFloorDnButton(floor) + control->getFloorUpButton(floor) \
            + control->getElevatorButton(elv, floor);
        if (is_btns_pressed == 0 || floor == control->getElevatorPosition(elv)) return -1;
        bool indicator = get_indicator(floor, elv), full = (is_elv_full_v1(elv, floor) || is_elvs_fulled[elv]);
        size_t distance = 1 / abs(control->getElevatorPosition(elv) - floor), time_exceed_out = time_exceed_btns_per_floors[floor];
        size_t time_exceed_in_elv = time_exceed_floors_in_elv[elv][floor];
        double a1 = 10000, a2 = m, a3 = n, a4 = k, a5 = f;
        return a1 * full + a2 * indicator + a3 * time_exceed_out + a4 * distance + a5 * time_exceed_in_elv;
    }

    // Функция для получения, следущего назначения лифта
    size_t get_next_dest(size_t elv, bool is_staying) {
        double max_priority = -100,  c_priority;
        int dest_floor = -1;
        size_t start = 0;
        size_t end = numberOfFloors;
        double pos = control->getElevatorPosition(elv);
        if (!is_staying) {
            if (control->getElevatorIndicator(elv) == ElevatorIndicator::up)
            {
                if ((int)pos == pos) start = pos;
                else start = pos + 1;
            }
            else if (control->getElevatorIndicator(elv) == ElevatorIndicator::down) 
            {
                if ((int)pos == pos) end = pos;
                else end = (int) pos + 1;
            }
        }
        for (size_t floor = start; floor < end; floor++) {
            c_priority = calc_priority_floor(floor, elv);

            //std::cout << "\tfloor = " << floor << " priority = " << c_priority << "\n";

            
            if (c_priority > max_priority) {
                max_priority = c_priority;
                dest_floor = floor;
            }
            //std::cout << max_priority << "\n";
        }
        if (dest_floor == -1) {
            if (control->getElevatorIndicator(elv) == ElevatorIndicator::up) control->SetElevatorIndicator(elv, ElevatorIndicator::down);
            else control->SetElevatorIndicator(elv, ElevatorIndicator::up);
            return control->getElevatorDestination(elv);
        }
        if (dest_floor > control->getElevatorPosition(elv)) control->SetElevatorIndicator(elv, ElevatorIndicator::up);
        else control->SetElevatorIndicator(elv, ElevatorIndicator::down);
        return dest_floor;
    }

    // Функция для определения заполненности лифта
    bool is_elv_full_v1(size_t elv, size_t floor) {
        static int passengers = 0;
        passengers -= control->getElevatorButton(elv, floor);
        passengers += (control->getFloorDnButton(floor) || control->getFloorUpButton(floor));

        if (passengers == elevatorCapacity) {
            passengers = 0;
            return true;
        }
        return false;
    }

    // Функция для определения заполненности лифта, возвращает время для проверки нажата ли кнопка
    size_t get_time_doors_closing(size_t elv) {
        static bool flag = false;
        size_t time = -1000;
        if (flag == false && control->isElevatorDoorsClosing(elv)) flag = true;
        if (flag) { 
            time = control->getCurrentTime(); 
            flag = false;
        }
        return time + control->timeClosing + 2;
    }

    // Функция для заезда на все попутные этажи
    void on_the_way_check(size_t elv) {
        if (!control->isElevatorStaying(elv)) {
            if (control->getElevatorIndicator(elv) == ElevatorIndicator::up) {
                for (size_t floor = (int)control->getElevatorPosition(elv) + 1; floor < control->getElevatorDestination(elv); floor++) {
                    if (control->getFloorUpButton(floor))
                    {
                        control->SetElevatorDestination(elv, floor);
                        break;
                    }

                }
            }
            else if (control->getElevatorIndicator(elv) == ElevatorIndicator::down) {
                for (size_t floor = std::max(round(control->getElevatorPosition(elv)) - 1, 1.0); floor > control->getElevatorDestination(elv); floor--) {
                    if (control->getFloorDnButton(floor))
                    {
                        control->SetElevatorDestination(elv, floor);
                        break;
                    }
                }
            }
        }
    }
};

// УДАЛИТЬ!!!!!
void writeInFile() {
    std::string fname = "mnk.txt";
    std::ofstream fout;
    fout.open(fname);
    fout << m << " " << n << " " << k <<  " " << f << std::endl;
    fout.close();
}

void CONTROLSYSTEM(Control& control, myParams& params);



int main_(int argc, char** argv)
{
    Control control(numberOfFloors, numberOfElevators, elevatorCapacity);
    control.ReadTimeTable("TimeTable/timetable125.csv");
    myParams params;
    do
    {
        control.MakeStep();

        CONTROLSYSTEM(control, params);
        

        //control.PrintElevatorState(0, "fileElev0.txt"); //Вывод состояния лифта #0 в файл
        //control.PrintElevatorState(1, "fileElev1.txt"); //Вывод состояния лифта #1 в файл

        //control.PrintButtonsState("fileButtons.txt");   //Вывод состояния кнопок в файл

        //control.PrintPassengerState("filePassengers.txt");  //Вывод статистики пассажиров в файл

    } while (control.getCurrentTime() <= maxTime);

    control.PrintStatistics(true, "Statistics.txt");
    writeInFile();
    return 0;
}
int main() {
    for (int m0 = 10; m0 < 300; m0 += 5) {
        for (int n0 = 10; n0 < 300; n0 += 5) {
            for (int k0 = 10; k0 < 500; k0 += 5) {
                for (int f0 = 10; f0 < 300; f0 += 5) {
                    m = m0;
                    n = n0;
                    k = k0;
                    f = f0;
                    main_(0, 0);
                }
            }
        }
    }
    return 0;

}

void CONTROLSYSTEM(Control& control, myParams& params)
{
    if (control.getCurrentTime() == 1)
    {
        params.init();
        params.control = &control;
        int floor_per_elv = numberOfFloors / numberOfElevators;
        for (size_t elv = 0; elv < numberOfElevators; elv++) {
            control.SetElevatorDestination(elv, elv * floor_per_elv);
        }
    }

    if (!params.started)
    {
        size_t nUp = std::count(control.getFloorUpButtons().begin(), control.getFloorUpButtons().end(), true);
        size_t nDn = std::count(control.getFloorDnButtons().begin(), control.getFloorDnButtons().end(), true);

        if (nUp + nDn > 0)
        {
            params.started = true;
        }
    }

    for (size_t elv = 0; elv < numberOfElevators; ++elv)
    {
        params.time_exceed_increment(elv);

        size_t time_to_check = params.get_time_doors_closing(elv);
        if (time_to_check > 0) {
            size_t last_floor = round(control.getElevatorPosition(elv));
            if (control.getCurrentTime() == time_to_check \
                && (control.getFloorDnButton(last_floor) + control.getFloorUpButton(last_floor)) > 0) params.is_elvs_fulled[elv] = true;
        }
        if (control.isElevatorStaying(elv)) control.SetElevatorIndicator(elv, ElevatorIndicator::both);
        params.on_the_way_check(elv);
        if ((params.started) && (control.isElevatorAchievedDestination(elv)))
        {
            size_t pos = control.getElevatorPosition(elv);
            if (((control.getFloorDnButton(pos) || control.getFloorUpButton(pos)) - control.getElevatorButton(elv, pos)) < 0)               params.is_elvs_fulled[elv] = false;
            size_t global_dest = params.get_next_dest(elv, control.isElevatorAchievedDestination(elv));

            control.unsetDnButton(pos);
            control.unsetUpButton(pos);
            params.time_exceed_btns_per_floors[pos] = 0;
            params.time_exceed_floors_in_elv[elv][pos] = 0;
            control.SetElevatorDestination(elv, global_dest);

        }
    }
}
