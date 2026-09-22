"""Domain-boundary matching with one set lookup per hostname suffix."""


def is_blocked(host, blocked_hosts):
    host = host.lower().rstrip('.')
    while host:
        if host in blocked_hosts:
            return True
        _, separator, host = host.partition('.')
        if not separator:
            break
    return False
