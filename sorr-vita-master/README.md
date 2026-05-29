[![Build PSVita](https://github.com/humbertodias/sorr-vita/actions/workflows/deploy.yml/badge.svg)](https://github.com/humbertodias/sorr-vita/actions/workflows/deploy.yml)

# SoRR

### Streets of Rage Remake

This is port of the Streets of Rage Remake (BennuGD engine) game to PSVita.

## Installing

**[For Linux/Windows/macOS]**  
You can download VPK-package from the [releases](https://github.com/isage/sorr-vita/releases) section.  
Install python3.   
Obtain SoRR 5.2 data and copy it into `data` directory in this repository.  
Run `python prepare.py`, this should remove unused files and unpack data file.  
Copy contents of `data/` to `ux0:/data/sorr/`.  
Install and run VPK.
