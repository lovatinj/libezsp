/**
 * @file ash-codec.h
 *
 * @brief ASH protocol decoder/encoder
 **/

#pragma once

#include <cstdint>
#include <memory>	// For std::unique_ptr

#include "spi/TimerBuilder.h"
#include "spi/ByteBuffer.h"
#include "ezsp/enum-generator.h"

namespace NSEZSP {

class CAshCallback;     /* Forward declaration */

#define ASH_INFO(XX) \
	XX(ASH_RESET_FAILED,=1) \
	XX(ASH_ACK,) \
	XX(ASH_NACK,) \
	XX(ASH_STATE_DISCONNECTED,) \
	XX(ASH_STATE_CONNECTED,) \

class AshCodec : protected NSSPI::ITimerVisitor
{
public:
	/**
	 * @brief Current state for the ASH decoder
	 *
	 * @note The lines above describes all states known in order to build both an enum and enum-to-string/string-to-enum methods
	 *       In this macro, XX is a placeholder for the macro to use for building.
	 *       We start numbering from 1, so that 0 can be understood as value not found for enum-generator.h
	 * @see enum-generator.h
	 */
	DECLARE_ENUM(EAshInfo, ASH_INFO);

	AshCodec() = delete; /* Construction without arguments is not allowed */

    /**
     * @param ipCb Callback invoked on ASH state change
     * @param i_timer_builder Timer builder object used to generate timers
     */
	AshCodec(CAshCallback *ipCb, const NSSPI::TimerBuilder& i_timer_builder);

	AshCodec(const AshCodec&) = delete; /* No copy construction allowed */

	AshCodec& operator=(AshCodec) = delete; /* No assignment allowed */

	/**
	 * @brief Check whether we are in ASH connected state or not
	 * 
	 * @return true if we are in ASH connected state
	 */
	bool isInConnectedState() const;

    NSSPI::ByteBuffer resetNCPFrame(void);

    NSSPI::ByteBuffer AckFrame(void);

    NSSPI::ByteBuffer DataFrame(NSSPI::ByteBuffer i_data);

    NSSPI::ByteBuffer decode(NSSPI::ByteBuffer &i_data);

	/**
	 * @brief Compute an ASH CRC16 on a speficied buffer
	 *
	 * @param buf The raw data on which we will compute the CRC16
	 *
	 * @return The 16-bit computed CRC
	 */
	static uint16_t computeCRC(const NSSPI::ByteBuffer& buf);

	/**
	 * @brief Remove ASH byte stuffing on an ASH raw stream
	 *
	 * @param i_data The raw stream from which byte stuffing will be removed
	 *
	 * @return An ASH payload with byte stuffing removed
	 */
	static NSSPI::ByteBuffer removeByteStuffing(const NSSPI::ByteBuffer& i_data);

	/**
	 * @brief Apply ASH byte stuffing to an ASH payload
	 *
	 * @param i_data The payload to which byte stuffing will be added
	 *
	 * @return A byte stuffed raw stream for transmission over a serial link
	 */
	static NSSPI::ByteBuffer addByteStuffing(const NSSPI::ByteBuffer& i_data);

	/**
	 * @brief Apply ASH randomisation (XORed LFSR) to an ASH payload
	 *
	 * @param i_data The payload to randomize
	 *
	 * @return A randomized buffer for transmission over a serial link
	 */
	static NSSPI::ByteBuffer dataRandomize(const NSSPI::ByteBuffer& i_data, uint8_t start = 0);

protected:
	void trigger(NSSPI::ITimer* triggeringTimer);

private:
	/**
	 * @brief Update state from the flag byte in the ASH message
	 */
	void decode_flag(NSSPI::ByteBuffer &lo_msg);

public:
	CAshCallback *pCb;
private:
	uint8_t ackNum; /*!< The sequence number of the next frame we are expecting, meaning we acknowlegde reception of the the last frame received with sequence number ackNum-1 */
	uint8_t frmNum; /*!< The sequence number of the next data frame originated by us */
	uint8_t ezspSeqNum;	/*!< FIXME: should be moved out of ASH, this is EZSP specific. The EZSP sequence number (wrapping 0-255 counter) */
	bool stateConnected;	/*!< Are we currently in connected state? (meaning we have an active working ASH handshake between host and NCP) */
	std::unique_ptr<NSSPI::ITimer> ackTimer;	/*!< A timer checking acknowledgement of the initial RESET (if !stateConnected) of the last ASH DATA frame (if stateConnected) */
	NSSPI::ByteBuffer in_msg; /*!< Currently accumulated buffer */
};

/**
 * @brief Interface of a ASH state change callback
 *
 * Objects that want to receive ASH state change should derive from this class and override method ashCbInfo
 */
class CAshCallback {
public:
	/**
	 * @brief Destructor
	 */
	virtual ~CAshCallback() = default;

	/**
	 * @brief Callback method invoked on ASH state change
	 *
	 * @param info The new ASH state
	 */
	virtual void ashCbInfo(AshCodec::EAshInfo info) = 0;
};

} // namespace NSEZSP
