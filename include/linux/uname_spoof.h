/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_UNAME_SPOOF_H
#define _LINUX_UNAME_SPOOF_H

#ifdef CONFIG_UNAME_SPOOF
bool uname_spoof_should_spoof(void);
const char *uname_spoof_get_release(void);
#else
static inline bool uname_spoof_should_spoof(void) { return false; }
static inline const char *uname_spoof_get_release(void) { return NULL; }
#endif

#endif /* _LINUX_UNAME_SPOOF_H */
