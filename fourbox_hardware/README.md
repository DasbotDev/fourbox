ros2 run teleop_twist_keyboard teleop_twist_keyboard   --ros-args   -p stamped:=true   -r /cmd_vel:=/fourbox_mecanum_controller/reference

# Fourbox Hardware Interface for ROS2 Jazzy

**ROS2 Control hardware interface** для **Mecanum колёсного робота Fourbox** с управлением по **UART/Arduino**.

Поддерживает **полное 4D движение**: вперед/назад, влево/вправо, поворот на месте, диагональ.

## Быстрый старт

```bash
# 1. Клонировать и собрать
cd ~/ros2_ws/src
git clone https://github.com/DasbotDev/fourbox.git
cd ~/ros2_ws && colcon build --packages-select fourbox_hardware

# 2. Подключить Arduino (/dev/ttyUSB0)
# 3. Запустить
source install/setup.bash
ros2 launch fourbox_hardware bringup.launch.py

# 4. Управление с помощью команд
ros2 topic pub /cmd_vel geometry_msgs/msg/TwistStamped "{linear: {x: 0.5}}" --rate 10
ros2 topic pub /cmd_vel geometry_msgs/msg/TwistStamped "{angular: {z: 1.0}}" --rate 10

# 4.1 Управление с помощью клавиатуры
ros2 run teleop_twist_keyboard teleop_twist_keyboard   --ros-args   -p stamped:=true   -r /cmd_vel:=/fourbox_mecanum_controller/reference
```

## Структура пакета

```
fourbox_hardware/
├── config/
│   └── controllers.yaml          # Параметры mecanum контроллера
├── include/fourbox_hardware/
│   └── fourbox_hardware.hpp      # Hardware interface
├── launch/
│   └── bringup.launch.py         # Полный запуск
└── CMakeLists.txt / package.xml
```

## Конфигурация

### **UART параметры** `Описаны в ros2_control.xacro`
```xml
    <param name="serial_port">/dev/ttyUSB0</param>          <!-- Имя последовательного порта -->
    <param name="baudrate">115200</param>                   <!-- Скорость -->
    <param name="wheel_radius">0.1</param>                  <!-- Радиус колеса [м] -->
    <param name="wheel_separation_horizontal">0.4</param>   <!-- Расстояние между колесами -->
    <param name="wheel_separation_vertical">0.5</param>     <!-- Расстояние между осями -->
    <param name="encoder_ppr">988</param>                   <!-- Импульсов/оборот -->

    <!-- ros2_control.xacro берет параметры из properties.xacro -->
```

### **Mecanum контроллер** `config/controllers.yaml`
```yaml
fourbox_mecanum_controller:
  type: mecanum_drive_controller/MecanumDriveController
  front_left_wheel: base_front_left_wheel_joint
  front_right_wheel: base_front_right_wheel_joint
  rear_left_wheel: base_rear_left_wheel_joint
  rear_right_wheel: base_rear_right_wheel_joint
```

## Arduino протокол

```
Запрос энкодеров:  "e\n"
Ответ:             "e,1234,5678,-9012,3456\n"  # 4 энкодера

Управление RPM:    "v,48,48,-48,48\n"
Формат:            v,[-255-255],[-255-255],[-255-255],[-255-255]\n
```

## Управление

| Команда | `/cmd_vel` | Ожидаемый эффект |
|---------|------------|------------------|
| Вперед | `{linear: {x: 0.5}}` | `v,50,50,50,50` |
| Влево | `{linear: {y: 0.5}}` | `v,50,-50,50,-50` |
| Поворот вправо | `{angular: {z: 1.0}}` | `v,50,-50,-50,50` |

## Топики и сервисы

```
/joint_states                            # Позиции/скорости колёс
/odom                                    # Одометрия (от Mecanum контроллера)
fourbox_mecanum_controller/reference     # Управление Twist
/controller_manager/...                 # ros2_control сервисы
```

## Устранение неисправностей

### **Контроллеры не активируются**
```
[ERROR] Switch controller timed out
ros2 launch fourbox_hardware bringup.launch.py
```

### **Робот не поворачивает**
```
1. Проверить порядок колёс в controllers.yaml
2. Добавить лог в write():
   RCLCPP_INFO(..., "VEL[%.2f,%.2f,%.2f,%.2f]", cmd_velocities_[0], ...);
3. Тест: ros2 topic pub /cmd_vel ... "{angular: {z: 1.0}}"
```

### **Не видит Arduino**
```bash
ls -l /dev/ttyUSB*          # Порт существует?
sudo chmod 666 /dev/ttyUSB0  # Права доступа
stty -F /dev/ttyUSB0 115200 # Baudrate
```

## Сборка и установка

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone https://github.com/DasbotDev/fourbox.git
cd ~/ros2_ws

# Зависимости
rosdep install --from-paths src --ignore-src -r -y

# Сборка
colcon build --packages-select fourbox_hardware

# Source
source install/setup.bash
```

## Зависимости

| Пакет | Версия | Назначение |
|-------|--------|------------|
| `ros-jazzy-hardware-interface` | ≥1.0 | ros2_control |
| `ros-jazzy-mecanum-drive-controller` | ≥1.0 | Mecanum |
| `ros-jazzy-controller-manager` | ≥1.0 | Загрузка контроллеров |

## Аппаратная часть

```
Fourbox Mecanum Robot:
├── 4x Mecanum колеса (45°/135°)
├── 4x Энкодер (988 PPR)
├── Arduino/ESP32 (UART 115200)
└── /dev/ttyUSB0
```

## Лицензия

[Apache-2.0](LICENSE) © 2026

## Контакты

- GitHub: [Issues](https://github.com/DasbotDev/fourbox/issues)
- Telegram: [@dasbot_support](https://t.me/dasbot_support)

***

**Fourbox Hardware Interface** — готовое решение для **Mecanum роботов** на **ROS2 Jazzy** с **Arduino UART**! 🚀