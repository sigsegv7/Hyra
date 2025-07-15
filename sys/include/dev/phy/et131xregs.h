/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Please refer to share/docs/hw/et131x.txt
 */

#ifndef _PHYS_ET131XREGS_H_
#define _PHYS_ET131XREGS_H_

#include <sys/types.h>

#define MAC_CFG1_SOFTRST        0x80000000  /* Soft reset */
#define MAC_CFG1_SIMRST         0x40000000  /* SIM reset */
#define MAC_CFG1_RESET_RXMC     0x00080000  /* RX MC reset */
#define MAC_CFG1_RESET_TXMC     0x00040000  /* TX MC reset */
#define MAC_CFG1_RESET_RXFUNC   0x00020000  /* RX func reset */
#define MAC_CFG1_RESET_TXFUNC   0x00010000  /* TX func reset */

#define PAD_N(N, NAME) uint8_t (NAME)[(N)]

/*
 * ET131X global registers
 */
struct global_regs {
    uint32_t txq_start;
    uint32_t txq_end;
    uint32_t rxq_start;
    uint32_t rxq_end;
    uint32_t pm_csr;
    uint32_t unused;
    uint32_t istat;
    uint32_t imask;
    uint32_t ialias_clr_en;
    uint32_t istat_alias;
    uint32_t sw_reset;
    uint32_t slv_timer;
    uint32_t msi_config;
    uint32_t loopback;
    uint32_t watchdog_timer;
};

/*
 * ET131X TX DMA registers
 */
struct txdma_regs {
    uint32_t csr;
    uint32_t pr_base_hi;
    uint32_t pr_base_lo;
    uint32_t pr_num_des;
    uint32_t txq_wr_addr;
    uint32_t txq_wr_addr_ext;
    uint32_t txq_rd_addr;
    uint32_t dma_wb_base_hi;
    uint32_t dma_wb_base_lo;
    uint32_t service_request;
    uint32_t service_complete;
    uint32_t cache_rd_index;
    uint32_t cache_wr_index;
    uint32_t tx_dma_error;
    uint32_t des_abort_cnt;
    uint32_t payload_abort_cnt;
    uint32_t wb_abort_cnt;
    uint32_t des_timeout_cnt;
    uint32_t payload_timeout_cnt;
    uint32_t wb_timeout_cnt;
    uint32_t des_error_cnt;
    uint32_t payload_err_cnt;
    uint32_t wb_error_cnt;
    uint32_t dropped_tlp_cnt;
    uint32_t new_service_complete;
    uint32_t ether_pkt_cnt;
};

/*
 * ET131X RX DMA registers
 */
struct rxdma_regs {
    uint32_t csr;
    uint32_t dma_wb_base_lo;
    uint32_t dma_wb_base_hi;
    uint32_t num_pkt_done;
    uint32_t max_pkt_time;
    uint32_t rxq_rd_addr;
    uint32_t rxq_rd_addr_ext;
    uint32_t rxq_wr_addr;
    uint32_t psr_base_lo;
    uint32_t psr_base_hi;
    uint32_t psr_num_des;
    uint32_t psr_avail_offset;
    uint32_t psr_full_offset;
    uint32_t psr_access_index;
    uint32_t psr_min_des;
    uint32_t fbr0_base_lo;
    uint32_t fbr0_base_hi;
    uint32_t fbr0_num_des;
    uint32_t fbr0_avail_offset;
    uint32_t fbr0_full_offset;
    uint32_t fbr0_rd_index;
    uint32_t fbr0_min_des;
    uint32_t fbr1_base_lo;
    uint32_t fbr1_base_hi;
    uint32_t fbr1_num_des;
    uint32_t fbr1_avail_offset;
    uint32_t fbr1_full_offset;
    uint32_t fbr1_rd_index;
    uint32_t fbr1_min_des;
};

/*
 * ET131X TX MAC registers
 */
struct txmac_regs {
    uint32_t ctl;
    uint32_t shadow_ptr;
    uint32_t err_cnt;
    uint32_t max_fill;
    uint32_t cf_param;
    uint32_t tx_test;
    uint32_t err;
    uint32_t err_int;
    uint32_t bp_ctrl;
};

/*
 * ET131X RX MAC registers
 */
struct rxmac_regs {
	uint32_t ctrl;
	uint32_t crc0;
	uint32_t crc12;
	uint32_t crc34;
	uint32_t sa_lo;
	uint32_t sa_hi;
	uint32_t mask0_word0;
	uint32_t mask0_word1;
	uint32_t mask0_word2;
	uint32_t mask0_word3;
	uint32_t mask1_word0;
	uint32_t mask1_word1;
	uint32_t mask1_word2;
	uint32_t mask1_word3;
	uint32_t mask2_word0;
	uint32_t mask2_word1;
	uint32_t mask2_word2;
	uint32_t mask2_word3;
	uint32_t mask3_word0;
	uint32_t mask3_word1;
	uint32_t mask3_word2;
	uint32_t mask3_word3;
	uint32_t mask4_word0;
	uint32_t mask4_word1;
	uint32_t mask4_word2;
	uint32_t mask4_word3;
	uint32_t uni_pf_addr1;
	uint32_t uni_pf_addr2;
	uint32_t uni_pf_addr3;
	uint32_t multi_hash1;
	uint32_t multi_hash2;
	uint32_t multi_hash3;
	uint32_t multi_hash4;
	uint32_t pf_ctrl;
	uint32_t mcif_ctrl_max_seg;
	uint32_t mcif_water_mark;
	uint32_t rxq_diag;
	uint32_t space_avail;
	uint32_t mif_ctrl;
	uint32_t err_reg;
};

struct mac_regs {
	uint32_t cfg1;
	uint32_t cfg2;
	uint32_t ipg;
	uint32_t hfdp;
	uint32_t max_fm_len;
	uint32_t rsv1;
	uint32_t rsv2;
	uint32_t mac_test;
	uint32_t mii_mgmt_cfg;
	uint32_t mii_mgmt_cmd;
	uint32_t mii_mgmt_addr;
	uint32_t mii_mgmt_ctrl;
	uint32_t mii_mgmt_stat;
	uint32_t mii_mgmt_indicator;
	uint32_t if_ctrl;
	uint32_t if_stat;
	uint32_t station_addr_1;
	uint32_t station_addr_2;
};

/* Global reset */
#define GBL_RESET_ALL	0x007F

/* MII management address */
#define MAC_MII_ADDR(PHY, REG) ((PHY) << 8 | (REG))

/* MAC management indications */
#define MAC_MGMT_BUSY 0x00000001
#define MAC_MGMT_WAIT 0x00000005

/* MAC management config values */
#define MAC_MIIMGMT_CLK_RST 0x00007

/* LED register defines */
#define PHY_LED2    0x1C

/* PCI config space offsets */
#define PCI_EEPROM_STATUS   0xB2
#define PCI_MAC_ADDRESS     0xA4

/*
 * LED control register 2 values
 */
#define LED_BLINK       0xD
#define LED_ON          0xE
#define LED_OFF         0xF
#define LED_ALL_OFF     0xFFFF

/*
 * LED register bit-shift constants
 *
 * Bits [3:0]:   100BASE-T LED
 * Bits [7:4]:   100BASE-TX LED
 * Bits [11:8]:  TX/RX LED
 * Bits [15:12]: Link LED
 */
#define LED_TXRX_SHIFT  8
#define LED_LINK_SHIFT  12

struct et131x_iospace {
#define _IO_PAD(NAME, REGSET) uint8_t NAME[4096 - sizeof(struct REGSET)]
    struct global_regs global;
    _IO_PAD(global_pad, global_regs);
    struct txdma_regs txdma;
    _IO_PAD(txdma_pad, txdma_regs);
    struct rxdma_regs rxdma;
    _IO_PAD(rxdma_pad, rxdma_regs);
    struct txmac_regs txmac;
    _IO_PAD(txmac_pad, txmac_regs);
    struct rxmac_regs rxmac;
    _IO_PAD(rxmac_pad, rxmac_regs);
    struct mac_regs mac;
    _IO_PAD(mac_pad, mac_regs);
    /* ... TODO - add more */
#undef  _IO_PAD
};

#endif  /* !_PHYS_ET131XREGS_H_ */
