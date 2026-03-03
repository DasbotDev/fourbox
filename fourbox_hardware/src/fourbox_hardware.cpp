#include <cmath>
#include <algorithm>
#include "fourbox_hardware/fourbox_hardware.hpp"

namespace fourbox_hardware {
    // Реализация основных методов класса
    hardware_interface::CallbackReturn FourboxHardware::on_init(const hardware_interface::HardwareInfo & info) {
        // Сообщение о начале инициализации.
        RCLCPP_INFO(rclcpp::get_logger("FourboxHardware"), "Initializing FourboxHardware...");

        // Переменная для хранения параметров из URDF
        hardware_info_ = info;

        // Читаем параметры из URDF.
        serial_port_ = hardware_info_.hardware_parameters.at("serial_port");
        baudrate_ = std::stoi(hardware_info_.hardware_parameters.at("baudrate"));
        wheel_radius_ = std::stod(hardware_info_.hardware_parameters.at("wheel_radius"));
        wheel_separation_horizontal_ = std::stod(hardware_info_.hardware_parameters.at("wheel_separation_horizontal"));
        wheel_separation_vertical_ = std::stod(hardware_info_.hardware_parameters.at("wheel_separation_vertical"));
        encoder_ppr_ = std::stod(hardware_info_.hardware_parameters.at("encoder_ppr"));
        
        // Вычисление параметров для рассчета скорости.
        counts_to_rad_ = 2.0 * M_PI / encoder_ppr_;
        
        // Сообщение с прочитанными параметрами.
        RCLCPP_INFO(rclcpp::get_logger("FourboxHardware"), 
            "Mecanum Config:\n"
            "  Port: %s\n"
            "  Wheel radius: %.3f m\n"
            "  Separation H (L-R): %.3f m\n"
            "  Separation V (F-R): %.3f m\n"
            "  Encoder PPR: %.0f",
            serial_port_.c_str(), wheel_radius_, wheel_separation_horizontal_, 
            wheel_separation_vertical_, encoder_ppr_);
        
        // Попытка открыть порт. В случае ошибки выкидывает ошибку инициализации.
        if (!open_serial()) {
            RCLCPP_FATAL(rclcpp::get_logger("FourboxHardware"), "Serial port not open.");
            return hardware_interface::CallbackReturn::ERROR;
        }

        // Возвращаем статус "SUCCESS" если все прошло хорошо.
        RCLCPP_INFO(rclcpp::get_logger("FourboxHardware"), "Serial port is open.");
        return hardware_interface::CallbackReturn::SUCCESS;
    }

    std::vector<hardware_interface::StateInterface> FourboxHardware::export_state_interfaces() {
        // Вектoр переменных для хранения интерфейсов состояния.
        std::vector<hardware_interface::StateInterface> state_interfaces;
        
        // Запись передача переменных из массивов интерфейсам.
        for (size_t i = 0; i < JOINTS_COUNT_; ++i) {
            state_interfaces.emplace_back(hardware_interface::StateInterface(
            hardware_info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_positions_[i]));
            state_interfaces.emplace_back(hardware_interface::StateInterface(
            hardware_info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &hw_velocities_[i]));
        }
        
        // Возвыращаем полученный вектор для дольнейшей обработки.
        return state_interfaces;
    }

    std::vector<hardware_interface::CommandInterface> FourboxHardware::export_command_interfaces() {
        // Вектoр переменных для хранения командных интерфейсов.
        std::vector<hardware_interface::CommandInterface> command_interfaces;

        // Запись передача переменных из массивов интерфейсам.
        for (size_t i = 0; i < JOINTS_COUNT_; ++i) {
            command_interfaces.emplace_back(hardware_interface::CommandInterface(
            hardware_info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &cmd_velocities_[i]));
        }

        // Возвращаем полученный вектор для дольнейшей обработки.
        return command_interfaces;
    }

    hardware_interface::return_type FourboxHardware::read(const rclcpp::Time &, const rclcpp::Duration &period) {
        // Переменная для хранения отметки периода.
        double delta_time = period.seconds();
        
        // Отправка запроса и получение данных в ответ.
        this->serial_write("e\n");
        std::string response = this->serial_readline(10);

        // RCLCPP_INFO(rclcpp::get_logger("FourboxHardware"), "Получено из порта: %s",response.c_str());

        // Проверяем содержание сообщения и парсим его в случае получения правильного.
        if (response.empty()) {
            return hardware_interface::return_type::OK;
        }
        if (response.find("e,") != 0) {
            RCLCPP_WARN(get_logger(), "The message from the microcontroller does not match the configuration!: '%s'", response.c_str());
            return hardware_interface::return_type::OK;
        }
        if (!response.empty() && response.find("e,") == 0) {
            std::vector<int> encoder_counts;
            std::stringstream ss(response.substr(2));
            std::string value;
            int counter = 0;

            while (std::getline(ss, value, ',')) {
                int int_value = std::stoi(value);
                hw_positions_[counter] = int_value * counts_to_rad_;
                hw_velocities_[counter] = (int_value - previous_encoders_value_[counter]) * counts_to_rad_ / delta_time;
                previous_encoders_value_[counter] = int_value;
                counter++;
            }
        } 
        return hardware_interface::return_type::OK;    
    }

    hardware_interface::return_type FourboxHardware::write(const rclcpp::Time &, const rclcpp::Duration &) {
        // Формирование сообщения для отправки
        std::string cmd = "v,";
        for(size_t i = 0; i < JOINTS_COUNT_; i++) {
            double rad_s = cmd_velocities_[i];
            double rpm_double = (rad_s * 60) / (2 * M_PI);
            int rpm_int = static_cast<int>(std::round(rpm_double));  
            cmd += std::to_string(rpm_int);
            if (i < JOINTS_COUNT_ - 1) cmd += ",";
        }
        cmd += "\n";
        // Отправка сообщения в последовательный порт
        this->serial_write (cmd);

        // RCLCPP_INFO(rclcpp::get_logger("FourboxHardware"), "Поступило для управления: %s",cmd.c_str());

        return hardware_interface::return_type::OK;
    }

    // Реализация методов для работы с последовательным портом.
    bool FourboxHardware::open_serial() {
        // Попытка открыть последовательный порт и получить его номер.
        serial_fd_ = open(serial_port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        // Проверка. Если номер порта не был получен возвращаем "false".
        if (serial_fd_ < 0) return false;

        // Объявление структуры для сохранения параметров последовательного порта.
        struct termios tty;
        //Запись параметров в структуру
        tcgetattr(serial_fd_, &tty);
        
        // Установка входной и выходной скорости последовательного порта.
        cfsetispeed(&tty, baudrate_ == 115200 ? B115200 : B57600); // Входная.
        cfsetospeed(&tty, baudrate_ == 115200 ? B115200 : B57600); // Выходная.
        
        // Настройка формата данных.
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_iflag &= ~IGNBRK;
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 1;
        tty.c_cc[VTIME] = 1;

        // Применяет параметры для последовательного порта немедленно и очищает буфер.
        tcsetattr(serial_fd_, TCSANOW, &tty);
        tcflush(serial_fd_, TCIOFLUSH);

        return true;
    }

    void FourboxHardware::close_serial() { 
        if (serial_fd_ >= 0) ::close(serial_fd_); 
    }

    bool FourboxHardware::serial_write(const std::string& data) {
        if (serial_fd_ < 0) return false;
        ssize_t bytes = ::write(serial_fd_, data.c_str(), data.length());
        tcdrain(serial_fd_);
        return bytes > 0;
    }

    std::string FourboxHardware::serial_readline(int timeout_ms) {
        // Настройка маски для последовательного порта.
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(serial_fd_, &fds);
        
        // Структура для указания периода ожидания.
        struct timeval tv = {0, timeout_ms * 1000};
        // Если пришли данные - обрабатываем их.
        if (select(serial_fd_ + 1, &fds, NULL, NULL, &tv) > 0) {
            char buf[256];
            int n = ::read(serial_fd_, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = 0;
                return std::string(buf);
            }
        }
        return "";
    }
} // namespace

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(fourbox_hardware::FourboxHardware, hardware_interface::SystemInterface)
