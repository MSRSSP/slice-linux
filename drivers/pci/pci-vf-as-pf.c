// SPDX-License-Identifier: GPL-2.0
/* kludge to instantiate a pre-existing SR-IOV VF as a fake PF on the PCI bus
 */

#include <linux/module.h>
#include <linux/pci.h>

static char *pf_ids;
module_param_named(pf, pf_ids, charp, S_IRUGO);
MODULE_PARM_DESC(pf, "Physical function IDs. One or more comma-separated [<domain>:]<bus>:<dev>.<func> (hex chars).");

static int vf_id;
module_param_named(vf, vf_id, int, S_IRUGO);
MODULE_PARM_DESC(vf, "Virtual function ID");

static int __init fake_vf_as_pf(int domain_id, int bus_id, int slot_id, int func_id)
{
	int rc;
	struct pci_bus *bus = pci_find_bus(domain_id, bus_id);
	if (bus == NULL) {
		pr_err("pci-pf-as-vf: PCI bus %x:%x not found\n", domain_id, bus_id);
		return -ENODEV;
	}

	pr_info("pci-pf-as-vf: faking %x:%x:%x.%x VF#%d\n",
		domain_id, bus_id, slot_id, func_id, vf_id);
	rc = pci_iov_fake_virt_as_phys(bus, PCI_DEVFN(slot_id, func_id), vf_id);
	if (rc != 0) {
		pr_err("pci-pf-as-vf: faking %x:%x:%x.%x VF#%d failed: %d",
			domain_id, bus_id, slot_id, func_id, vf_id, rc);
	}

	return rc;
}

static int __init vf_as_pf_init(void)
{ 
	const char* s = pf_ids;

	if (s == NULL || *s == '\0') {
		pr_err("pci-pf-as-vf: 'pf' parameter is required\n");
		return -EINVAL;
	}

	while (*s)
	{
		int rc, count;
		unsigned short domain_id, bus_id, slot_id, func_id;

		rc = sscanf(s, "%hx:%hx:%hx.%hx%n", &domain_id, &bus_id, &slot_id, &func_id, &count);
		if (rc != 4) {
			domain_id = 0;
			rc = sscanf(s, "%hx:%hx.%hx%n", &bus_id, &slot_id, &func_id, &count);
			if (rc != 3) {
				pr_err("pci-pf-as-vf: 'pf' parameter is invalid\n");
				return -EINVAL;
			}
		}

		rc = fake_vf_as_pf(domain_id, bus_id, slot_id, func_id);
		if (rc != 0)
		{
			return rc;
		}

		s += count;

		if (*s == ',') {
			s++;
		} else if (*s != '\0') {
			pr_err("pci-pf-as-vf: 'pf' parameter is invalid\n");
			return -EINVAL;
		}
	}

	return 0;
}

module_init(vf_as_pf_init); 

MODULE_LICENSE("GPL");
