#include <stdbool.h>

#include <zephyr/drivers/can.h>
#include <zephyr/random/random.h>

#include "can_sender.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_sender, CONFIG_CAN_SENDER_LOG_LEVEL);

#define NUM_TX_MSGS 3

static struct device *can_dev;

static void can_tx_callback(const struct device *dev, int error, void *user_data)
{
	LOG_DBG("Sent CAN frame with id=%d, error=%d", (int)(uintptr_t)user_data, error);
}

#define CAN_TX_TIMEOUT K_MSEC(100)

static void tx_send(uint32_t msg_id)
{
	struct can_frame frame = {0};

	frame.id = msg_id;
	frame.dlc = 8;
	for (int i = 0; i < frame.dlc; i++) {
		frame.data[i] = (uint8_t)sys_rand8_get();
	}

	int err = can_send(can_dev, &frame, CAN_TX_TIMEOUT, can_tx_callback, (void *)frame.id);
	if (err != 0) {
		LOG_ERR("failed to enqueue CAN frame (err %d)", err);
	}
}

struct tx_ctx {
	struct k_work work;
	struct k_timer timer;
	uint32_t msg_id;
};

static struct tx_ctx tx_ctxs[NUM_TX_MSGS];

static void tx_work_handler(struct k_work *work)
{
	struct tx_ctx *ctx = CONTAINER_OF(work, struct tx_ctx, work);
	tx_send(ctx->msg_id);
}

static void timer_handler(struct k_timer *timer_id)
{
	struct tx_ctx *ctx = CONTAINER_OF(timer_id, struct tx_ctx, timer);
	k_work_submit(&ctx->work);
}

int can_sender_init_custom(const struct device *dev, uint32_t msg_0_id, uint32_t msg_0_period_ms,
			   uint32_t msg_1_id, uint32_t msg_1_period_ms, uint32_t msg_2_id,
			   uint32_t msg_2_period_ms)
{
	if (!device_is_ready(dev)) {
		LOG_ERR("CAN device not ready");
		return -EIO;
	}

	can_dev = (struct device *)dev;

	uint32_t msg_ids[NUM_TX_MSGS] = {msg_0_id, msg_1_id, msg_2_id};
	uint32_t msg_periods[NUM_TX_MSGS] = {msg_0_period_ms, msg_1_period_ms, msg_2_period_ms};

	for (size_t i = 0; i < NUM_TX_MSGS; i++) {
		tx_ctxs[i].msg_id = msg_ids[i];
		k_work_init(&tx_ctxs[i].work, tx_work_handler);
		k_timer_init(&tx_ctxs[i].timer, timer_handler, NULL);
		k_timer_start(&tx_ctxs[i].timer, K_MSEC(msg_periods[i]), K_MSEC(msg_periods[i]));
	}

	return 0;
}

int can_sender_init(const struct device *dev)
{
	return can_sender_init_custom(
		dev, CONFIG_CAN_SENDER_TX_0_MSG_ID, CONFIG_CAN_SENDER_TX_0_MSG_PERIOD_MS,
		CONFIG_CAN_SENDER_TX_1_MSG_ID, CONFIG_CAN_SENDER_TX_1_MSG_PERIOD_MS,
		CONFIG_CAN_SENDER_TX_2_MSG_ID, CONFIG_CAN_SENDER_TX_2_MSG_PERIOD_MS);
}
