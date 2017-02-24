#include <linux/cache.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>

#define DRIVER_NAME "spi_mpu9250"
#define MPU9250_MAX_SPEED_HZ 4000000 /* 4Mhz */
#define MPU9250_SPEED_HZ 1000000     /* 1Mhz */
#define MPU9250_SPI_WORD_SIZE 8
#define MPU9250_PACKET_SIZE 20

typedef unsigned char mpu9250_addr_t;

struct mpu9250_drvdata {
  struct spi_device *spi;                                      /** spiデバイス用データ */
  struct mutex lock;                                           /** lock */
  unsigned char tx[MPU9250_PACKET_SIZE] ____cacheline_aligned; /** 送信データ */
  unsigned char rx[MPU9250_PACKET_SIZE] ____cacheline_aligned; /** 受信データ */
  struct spi_transfer xfer ____cacheline_aligned;              /** 転送用データ(spi_sync_transfer用) */
  struct spi_message msg ____cacheline_aligned;                /** 転送用データ(spi_sync用) */
};

static int spi_bus_num = 0;     /** bus番号  */
static int spi_chip_select = 0; /** チップセレクトピン番号  */

static struct spi_board_info mpu9250_info = {
    .modalias = "mpu9250",                /** モジュール名 */
    .max_speed_hz = MPU9250_MAX_SPEED_HZ, /** 通信速度:4Mhz */
    .bus_num = 0,                         /** バス番号 */
    .chip_select = 0,                     /** チップセレクトPIN番号 */
    .mode = SPI_MODE_2,                   /** SPIのモード  */
};

static void mpu9250_spi_transfer_write(struct mpu9250_drvdata *drvdata, mpu9250_addr_t addr, unsigned char data[], size_t size)
{
  mutex_lock(&drvdata->lock);

  if (spi_sync(drvdata->spi, &drvdata->msg)) {
    pr_info("spi_sync_transfer returned not zero\n");
  }

  mutex_unlock(&drvdata->lock);

  return;
}

static void mpu9250_spi_transfer_read(struct mpu9250_drvdata *drvdata, mpu9250_addr_t addr, unsigned char data[], size_t size)
{
  mutex_lock(&drvdata->lock);

  if (spi_sync(drvdata->spi, &drvdata->msg)) {
    pr_info("spi_sync_transfer returned not zero\n");
  }

  mutex_unlock(&drvdata->lock);

  return;
}

static int mpu9250_probe(struct spi_device *spi)
{
  pr_info("mpu9250 probe\n");

  spi->max_speed_hz = mpu9250_info.max_speed_hz;
  spi->mode = mpu9250_info.mode;
  spi->bits_per_word = MPU9250_SPI_WORD_SIZE;

  if (spi_setup(spi)) {
    pr_err("spi_setput returned error\n");
    return -ENODEV;
  }

  struct mpu9250_drvdata *data = kzalloc(sizeof(struct mpu9250_drvdata), GFP_KERNEL);

  if (data == NULL) {
    pr_err("%s:no memory\n", __func__);
    return -ENODEV;
  }

  data->spi = spi;

  mutex_init(&data->lock);

  data->xfer.tx_buf = data->tx;
  data->xfer.rx_buf = data->rx;
  data->xfer.len = MPU9250_PACKET_SIZE;
  data->xfer.cs_change = 0;
  data->xfer.delay_usecs = 0;
  data->xfer.speed_hz = MPU9250_SPEED_HZ;
  spi_message_init_with_transfers(&data->msg, &data->xfer, 1);
  spi_set_drvdata(spi, data);

  return 0;
}

static int mpu9250_remove(struct spi_device *spi)
{
  struct mpu9250_drvdata *data = (struct mpu9250_drvdata *)spi_get_drvdata(spi);

  kfree(data);
  pr_info("mpu9250 removed\n");

  return 0;
}

static struct spi_device_id mpu9250_id[] = {
    {"mpu9250", 0}, /* device id */
    {},
};

MODULE_DEVICE_TABLE(spi, mpu9250_id); /** device idの登録  */

static struct spi_driver mpu9250_driver = {
    .driver =
        {
            .name = DRIVER_NAME,
            .owner = THIS_MODULE,
        },
    .id_table = mpu9250_id,
    .probe = mpu9250_probe,
    .remove = mpu9250_remove,
};

/**
 * /dev/spidevX.Xを取り除く
 */
static void spi_remove_device(struct spi_master *master, unsigned int cs)
{
  char str[128];

  snprintf(str, sizeof(str), "%s.%u", dev_name(&master->dev), cs);

  struct device *dev = bus_find_device_by_name(&spi_bus_type, NULL, str);

  if (dev) {
    pr_info("Delete %s\n", str);
    device_del(dev);
  }

  return;
}

static int __init mpu9250_init(void)
{
  spi_register_driver(&mpu9250_driver);
  mpu9250_info.bus_num = spi_bus_num;
  mpu9250_info.chip_select = spi_chip_select;

  struct spi_master *master = spi_busnum_to_master(mpu9250_info.bus_num);
  if (!master) {
    pr_err("spi_busnum_to_master returned NULL\n");
    spi_unregister_driver(&mpu9250_driver);
    return -ENODEV;
  }

  /* デバイスツリーの削除 */
  spi_remove_device(master, mpu9250_info.chip_select);

  /* 新しくデバイスツリーの作成 */
  struct spi_device *spi_device = spi_new_device(master, &mpu9250_info);
  if (!spi_device) {
    pr_err("spi_new_device returne NULL\n");
    spi_unregister_driver(&mpu9250_driver);
    return -ENODEV;
  }

  return 0;
}

static void __exit mpu9250_exit(void)
{
  struct spi_master *master = spi_busnum_to_master(mpu9250_info.bus_num);

  if (master) {
    spi_remove_device(master, mpu9250_info.chip_select);
  } else {
    pr_info("mpu9250 remove error\n");
  }

  spi_unregister_driver(&mpu9250_driver);

  return;
}

module_init(mpu9250_init);
module_exit(mpu9250_exit);
module_param(spi_bus_num, int, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);     /** sysfsに公開  */
module_param(spi_chip_select, int, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR); /** sysfsに公開  */

MODULE_DESCRIPTION("mpu9250 device driver");
MODULE_VERSION("1.0");
MODULE_AUTHOR("Yohei MIYAMOTO, Xyzzyx Inc.");
MODULE_LICENSE("GPL v2");
