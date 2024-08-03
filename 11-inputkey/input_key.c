#include "input_key.h"

struct inputkey_dev key_dev;

static irqreturn_t key0_handler(int irq, void *dev_id) {
  mod_timer(&key_dev.timer, jiffies + msecs_to_jiffies(10));
  return IRQ_RETVAL(IRQ_HANDLED);
}

static void timer_callback(unsigned long arg) {
  // printk("timer start \n");
  int val = gpio_get_value(key_dev.keydesc.gpio_index);

  if (val == 0) {
    input_report_key(key_dev.inputdev, KEY_0, 1);
    input_sync(key_dev.inputdev);
  }

  else {
    input_report_key(key_dev.inputdev, KEY_0, 0);
    input_sync(key_dev.inputdev);
  }
}

void key_io_init(void) {
  u32 val = 0;
  int ret;
  u32 regdata[14];
  const char *str;
  struct property *proper;

  /* 获取设备节点 */
  key_dev.nd = of_find_node_by_path("/key");
  if (key_dev.nd == NULL) {
    printk("device node nost find!\r\n");
    return -EINVAL;
  } else {
    printk("device node find!\r\n");
  }

  /* 获取设备树的gpio子系统节点的属性,得到led的所使用的gpio编号 */
  key_dev.keydesc.gpio_index = of_get_named_gpio(key_dev.nd, "key-gpio", 0);
  if (key_dev.keydesc.gpio_index < 0) {
    printk("can't get key-gpio! \r\n");
    return -EINVAL;
  }
  printk("key-gpio num = %d\r\n", key_dev.keydesc.gpio_index);

  /* 设置该GPIO */
  gpio_request(key_dev.keydesc.gpio_index, "key0");
  gpio_direction_input(key_dev.keydesc.gpio_index);
  key_dev.keydesc.irq_num = irq_of_parse_and_map(key_dev.nd, 0);

  /* 配置中断 */
  key_dev.keydesc.handler = key0_handler;
  key_dev.keydesc.value = 0;
  memset(key_dev.keydesc.name, 0, sizeof(key_dev.keydesc.name));
  sprintf(key_dev.keydesc.name, "KEY0");
  request_irq(key_dev.keydesc.irq_num, key_dev.keydesc.handler,
              IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, key_dev.keydesc.name,
              &key_dev);

  /* 初始化定时器 */
  init_timer(&key_dev.timer);
  key_dev.timer.function = timer_callback;
  add_timer(&key_dev.timer);

  /* 初始化 input_dev */
  key_dev.inputdev = input_allocate_device();
  key_dev.inputdev->name = "key_input";
  key_dev.inputdev->evbit[0] =
      BIT_MASK(EV_KEY) | BIT_MASK(EV_REP); // 设置输入设备支持的事件类型
  input_set_capability(key_dev.inputdev, EV_KEY,
                       KEY_0); // 设置输入设备支持的按键

  /* 向内核注册 input_dev */
  ret = input_register_device(key_dev.inputdev);
  if (ret)
    printk("input_register_device failed!");
}

static int key_input_init(void) {
  key_io_init();
  return 0;
}

static void key_input_exit(void) {
  gpio_direction_output(key_dev.keydesc.gpio_index, 1);
  gpio_free(key_dev.devid);

  del_timer_sync(&key_dev.timer);

  free_irq(key_dev.keydesc.irq_num, &key_dev);

  input_unregister_device(key_dev.inputdev);
  input_free_device(key_dev.inputdev);
}

module_init(key_input_init);
module_exit(key_input_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A Simple Character Device Driver");
MODULE_VERSION("0.1");
