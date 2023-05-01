#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include "Control.h"

const size_t numberOfElevators = 2;

const size_t elevatorCapacity = 6;

const size_t maxFloor = 11;

const size_t numberOfFloors = maxFloor + 1;

const size_t maxTime = 6000;


struct myParams
{
    bool started = false;
    size_t* time_table = new size_t[numberOfFloors];
    size_t* time_elv_full = new size_t[numberOfElevators];
    bool* list_elv_full = new bool[numberOfElevators];
        
    ~myParams() {
        delete[] time_table, time_elv_full, list_elv_full;
    }


    void init_params() {

        for (int i = 0; i < numberOfElevators; ++i) {
             time_elv_full[i] = 0;
             list_elv_full[i] = false;
        }
        
        for (int i = 0; i < numberOfFloors; ++i) {
            time_table[i] = 0;
            
        }

    }

    void floorbutton_time_increment(Control &control) {
        for (int i = 0; i < numberOfFloors; ++i) {
            time_table[i] += control.getFloorDnButton(i) + control.getFloorUpButton(i);
        }
    }

    void floorbutton_time_increment_elevator(Control& control, size_t elv) {
        for (int i = 0; i < numberOfFloors; ++i) {
            time_table[i] += control.getElevatorButton(elv, i);
        }
    }

    void floorbutton_time_reset(Control& control, size_t elv) {
        size_t pos = control.getElevatorPosition(elv);
        time_table[pos] = 0;
    }



    double get_priority(Control& control, size_t elv, size_t floor) {
        // аргументы функции приоритета
        size_t distance = abs(control.getElevatorPosition(elv) - floor);
        size_t time_exceed = time_table[floor];
        ElevatorIndicator c_indi = control.getElevatorIndicator(elv);
        bool Indicator = 0;
        if ((floor > control.getElevatorPosition(elv) && c_indi == ElevatorIndicator::up) || (floor < control.getElevatorPosition(elv) \
            && c_indi == ElevatorIndicator::down)) Indicator = 1;
        int departure = 0;
        // если лифт заполнен и нажата кнопка в самом лифте
        if (is_elevator_full(control, elv, floor) && control.getElevatorButton(elv, floor)) departure = 1;

        double pr = 1000 * departure + 40 * Indicator + 40 * time_exceed + 75 * distance; // min = 44000

        return pr;
    }

    /*
    struct Elevator_priorities {
        size_t elv;
        double* elv_priorities;
        Elevator_priorities(size_t elv_n) {
            elv = elv_n;
            elv_priorities = new double[numberOfFloors];
            for (int i = 0; i < numberOfFloors; ++i) {
                elv_priorities[i] = 0;
            }
        }
        ~Elevator_priorities() { delete[] elv_priorities; };

        
    };
    */

    size_t get_target(Control& control, size_t elv) {
        size_t index = 0;
        size_t max = 0;
        for (size_t i = 0; i < numberOfFloors; ++i) {
            
            double tmp = get_priority(control, elv, i);
            if (tmp > max) {
                max = tmp;
                index = i;
            }
        }
        return index;
    }
    
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
        for (int i = floor + 1; i < numberOfFloors; i++) {
            if (u_btns[i] || elv_btns[i]) return i;
        }
        return floor;
    }

    size_t find_dnwn_dest(Control& control, size_t elv, size_t floor) {
        std::vector<bool> d_btns = control.getFloorDnButtons();
        std::vector<bool> elv_btns = control.getElevatorButtons(elv);
        for (int i = floor - 1; i >= 0; i--) {
            if (d_btns[i] || elv_btns[i]) return i;
        }
        return floor;
    }
   
    
    
    /*void is_elevator_full(Control& control, size_t elv) {

        if (time_elv_full[elv] == 0 && control.isElevatorDoorsClosing(elv)) {
            time_elv_full[elv] = control.getCurrentTime() + control.timeClosing + 3;
        }
    } 
    */
};


void CONTROLSYSTEM(Control& control, myParams& params);

int main(int argc, char** argv)
{
    Control control(numberOfFloors, numberOfElevators, elevatorCapacity);
    control.ReadTimeTable("TimeTable/timetable125.csv");

    //control.AddPassengerToQueue({  5, 5,  3, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({  6, 5, 10, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({  7, 5,  2, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({  8, 5,  8, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({  9, 5, 10, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({ 10, 5,  6, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({ 11, 5,  9, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({ 12, 5,  8, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({ 13, 5, 11, 300, 0.01, 0.20, 0.50 });
    //control.AddPassengerToQueue({ 14, 5, 10, 300, 0.01, 0.20, 0.50 });
    

    myParams params;
    do
    {
        control.MakeStep();
        CONTROLSYSTEM(control, params);

        //control.PrintElevatorState(0);                //Вывод состояния лифта #0 на экран
        //control.PrintElevatorState(1);                //Вывод состояния лифта #1 на экран

        control.PrintElevatorState(0, "fileElev0.txt"); //Вывод состояния лифта #0 в файл
        control.PrintElevatorState(1, "fileElev1.txt"); //Вывод состояния лифта #1 в файл

        //control.PrintButtonsState();                  //Вывод состояния кнопок на экран
        control.PrintButtonsState("fileButtons.txt");   //Вывод состояния кнопок в файл

        //control.PrintPassengerState();                    //Вывод статистики пассажиров на экран
        control.PrintPassengerState("filePassengers.txt");  //Вывод статистики пассажиров в файл

    } while (control.getCurrentTime() <= maxTime);

    //Печать итоговой статистики в конце работы симулятора   
    control.PrintStatistics(true, "Statistics.txt");

    return 0;
}

void CONTROLSYSTEM(Control& control, myParams& params) 
{
    if (control.getCurrentTime() == 1)
    {
        params.init_params();
       

        if (numberOfElevators % 2 == 0) {
            for (int i = 0; i < numberOfElevators / 2; i++) {
                control.SetElevatorDestination(i, maxFloor);
                control.SetElevatorIndicator(i, ElevatorIndicator::up);
            }
        }
        else {
            control.SetElevatorDestination(numberOfElevators / 2, numberOfFloors / 2);
            control.SetElevatorIndicator(numberOfElevators / 2, ElevatorIndicator::up);
            for (int i = 0; i < numberOfElevators / 2; i++) {
                control.SetElevatorDestination(i, maxFloor);
                control.SetElevatorIndicator(i, ElevatorIndicator::up);
            }

        }
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

    for (size_t elv = 0; elv < numberOfElevators; ++elv) {
       
        if ((params.started) && (control.isElevatorAchievedDestination(elv))) {
     
            params.floorbutton_time_reset(control, elv);
            //for (int i = 0; i < numberOfFloors; ++i) {
            //    std::cout << params.time_table[i] << " ";
            //}
            //std::cout << std::endl;

            size_t pos = (size_t)(control.getElevatorPosition(elv));
            control.unsetDnButton(pos);
            control.unsetUpButton(pos);
            size_t global_dest;
            global_dest = params.get_target(control, elv);

            if (global_dest > pos) {
                control.SetElevatorIndicator(elv, ElevatorIndicator::up);
            }

            else {
                control.SetElevatorIndicator(elv, ElevatorIndicator::down);
            }

            control.SetElevatorDestination(elv, global_dest);

            /*
            if (!(params.is_elevator_full(control, elv, pos))) {

                if (control.getElevatorIndicator(elv) == ElevatorIndicator::up || control.getElevatorIndicator(elv) == ElevatorIndicator::both) {
                    global_dest = params.find_upper_dest(control, elv, pos);

                    if (global_dest == pos && params.find_dnwn_dest(control, elv, pos) != pos) control.SetElevatorIndicator(elv, ElevatorIndicator::down);
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
            */

            control.SetElevatorDestination(elv, global_dest);
        }
        params.floorbutton_time_increment_elevator(control, elv);


    }

    params.floorbutton_time_increment(control);
     

        /* когда-нибудь оно заработает
        params.is_elevator_full(control, elv);

        if (params.time_elv_full[elv] == control.getCurrentTime()) {

            int nearest_floor = round((control.getElevatorPosition(elv)));
            std::cout << std::endl << "nearest floor: " << nearest_floor << std::endl << "current pos: " << \
                control.getElevatorPosition(elv) << std::endl;


            if ((control.getFloorUpButton(nearest_floor)) || (control.getFloorDnButton(nearest_floor))) { 

                std::cout << "somtegin" << "; " << control.getCurrentTime() << "second" << std::endl;
            
                params.list_elv_full[elv] = true;
                params.time_elv_full[elv] = 0;
            }
        }
    }
    */

    /*
     if (control.getCurrentTime() < 5)
         control.SetElevatorDestination(1, 2);
     else
         control.SetElevatorDestination(1, 0);
    */ 
}