#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <memory>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

class KeyReader
{
public:
  KeyReader()
  {
    tcgetattr(STDIN_FILENO, &orig_);
    struct termios raw = orig_;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  }

  ~KeyReader() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_); }

  int getKey()
  {
    unsigned char c;
    struct timeval tv = {0, 1000};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return -1;
    if (read(STDIN_FILENO, &c, 1) <= 0) return -1;
    if (c != 27) return c;

    tv.tv_usec = 2000;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 27;

    char seq[2];
    ssize_t n = read(STDIN_FILENO, seq, 2);
    if (n < 2 || seq[0] != '[') return -1;
    switch (seq[1]) {
      case 'A': return kUp;
      case 'B': return kDown;
      case 'C': return kRight;
      case 'D': return kLeft;
      default:  return -1;
    }
  }

  static constexpr int kUp    = 0x100;
  static constexpr int kDown  = 0x101;
  static constexpr int kLeft  = 0x102;
  static constexpr int kRight = 0x103;

private:
  struct termios orig_;
};

class KeyboardControlNode : public rclcpp::Node
{
public:
  KeyboardControlNode()
    : Node("keyboard_control"),
      key_reader_(std::make_unique<KeyReader>())
  {
    pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/car/cmd_vel", 10);
    RCLCPP_INFO(this->get_logger(),
R"(╔══════════════════════════════════════════╗
║         SPORTS CAR KEYBOARD CONTROLS     ║
╠══════════════════════════════════════════╣
║  W/S   accelerate / reverse             ║
║  A/D   steer left / right               ║
║  ↑/↓   faster / slower speed preset     ║
║  SPACE brake                            ║
║  ESC / ^C  exit                         ║
╚══════════════════════════════════════════╝)");
    timer_ = this->create_wall_timer(20ms, [this]() { update(); });
  }

private:
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::unique_ptr<KeyReader> key_reader_;

  double speed_        = 0.0;
  double steer_        = 0.0;
  double speed_preset_ = 2.0;
  double last_preset_  = 2.0;

  static constexpr double kMaxSpeed = 4.0;
  static constexpr double kMaxSteer = 2.5;

  void update()
  {
    bool w = false, s = false, a = false, d = false;
    while (true) {
      int c = key_reader_->getKey();
      if (c < 0) break;
      switch (c) {
        case 'w': case 'W': w = true; break;
        case 's': case 'S': s = true; break;
        case 'a': case 'A': a = true; break;
        case 'd': case 'D': d = true; break;
        case KeyReader::kUp:   speed_preset_ = std::min(kMaxSpeed, speed_preset_ + 0.5); break;
        case KeyReader::kDown: speed_preset_ = std::max(0.5, speed_preset_ - 0.5); break;
        case ' ': speed_ = 0.0; steer_ = 0.0; break;
        case 3: case 27: rclcpp::shutdown(); return;
        default: break;
      }
    }
    if (std::abs(speed_preset_ - last_preset_) > 0.01) {
      last_preset_ = speed_preset_;
      RCLCPP_INFO(this->get_logger(), "Speed preset: %.1f m/s", speed_preset_);
    }
    if (w)  speed_ = speed_preset_;
    else if (s) speed_ = -speed_preset_ * 0.6;
    else if (speed_ != 0) { speed_ *= 0.90; if (std::abs(speed_) < 0.01) speed_ = 0; }
    if (a) steer_ = kMaxSteer;
    else if (d) steer_ = -kMaxSteer;
    else { steer_ *= 0.85; if (std::abs(steer_) < 0.01) steer_ = 0; }
    auto msg = geometry_msgs::msg::Twist();
    msg.linear.x = speed_;
    msg.angular.z = steer_;
    pub_->publish(msg);
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<KeyboardControlNode>());
  rclcpp::shutdown();
  return 0;
}
