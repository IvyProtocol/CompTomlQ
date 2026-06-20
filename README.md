## A simple TOML Parser 
This repository is made only to convert TOML specific Group and Keys to respectively convert them to Bash Keys.

# Basic Key-Value Pairs
The foundation of TOML is simple assignment.

TOML:
```toml
name = "TOML Project"
enabled = true
version = 1.0

max_connections = 5000
timeout = 30.5
```

Bash:
```bash
export NAME="TOML Project"
export ENABLED=true
export VERSION=1.0

export MAX_CONNECTIONS=5000
export TIMEOUT=30.5
```

# Tables
Tables are used to group keys. You define a table with a header in square brackets.

TOML: 
```toml
[database]
server = "192.168.1.1"
ports = [8000, 8001, 8002]
connection_max = 5000
enabled = true

[servers.alpha]
ip = "10.0.0.1"
role = "frontend"
```

Bash:
```bash
export DATABASE_SERVER="192.168.1.1"
export DATABASE_PORTS=(8000 8001 8002)
export DATABASE_CONNECTION_MAX=5000
export DATABASE_ENABLED=true

export SERVERS_ALPHA_IP="10.0.0.1"
export SERVERS_ALPHA_ROLE="frontend"
```

Note: You have to handle Array differently than these keys. 
## 
No array of Tables and Inline Tables yet. In future, I hope.
