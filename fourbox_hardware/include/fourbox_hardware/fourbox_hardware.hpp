#pragma once

#include <array>
#include <string>
#include <vector>
#include <fcntl.h>      
#include <unistd.h>     
#include <termios.h>    
#include <cstring>
#include <rclcpp/rclcpp.hpp>
#include <hardware_interface/system_interface.hpp>
#include <hardware_interface/types/hardware_interface_type_values.hpp>

// Пространство имен для контроллера
namespace fourbox_hardware {
    // Наследование от оригинального класса
    class FourboxHardware : public hardware_interface::SystemInterface {
        public:
            // Переопределяем функции родительского класса указав это явно (override)
            hardware_interface::CallbackReturn on_init(const hardware_interface::HardwareInfo & info) override;
            std::vector<hardware_interface::StateInterface> export_state_interfaces() override;
            std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;
            hardware_interface::return_type read(const rclcpp::Time &, const rclcpp::Duration &period) override;
            hardware_interface::return_type write(const rclcpp::Time &, const rclcpp::Duration &) override;

        private:
            // Параметры последовательного порта (получаем из URDF)
            int serial_fd_ = -1;
            int baudrate_ = 115200;
            std::string serial_port_ = "/dev/ttyUSB0";
            
            // Параметры робота (получаем из URDF)
            static constexpr int JOINTS_COUNT_ = 4;
            double wheel_radius_ = 0.1;
            double wheel_separation_horizontal_ = 0.4;
            double wheel_separation_vertical_ = 0.5;
            double encoder_ppr_ = 988.0;
            double counts_to_rad_;

            // Параметры для вычислений
            hardware_interface::HardwareInfo hardware_info_;
            std::array<long, JOINTS_COUNT_> previous_encoders_value_{};
            std::array<double, JOINTS_COUNT_> hw_positions_{0.0};
            std::array<double, JOINTS_COUNT_> hw_velocities_{0.0};
            std::array<double, JOINTS_COUNT_> cmd_velocities_{0.0};

            // Методы последовательного порта
            bool open_serial();
            void close_serial();
            bool serial_write(const std::string& data);
            std::string serial_readline(int timeout_ms = 10);         
    };        
} // namespace