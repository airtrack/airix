#ifndef PCI_H
#define PCI_H

enum pci_config_port
{
    PCI_CONFIG_ADDRESS_PORT = 0xcf8,
    PCI_CONFIG_DATA_PORT = 0xcfc
};

enum pci_class
{
    PCI_CLASS_MASS_STORAGE = 0x1
};

enum pci_subclass
{
    PCI_SUBCLASS_IDE = 0x1
};

void pci_detecting_devices();

#endif /* PCI_H */
