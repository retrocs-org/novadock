#!/usr/bin/env python3
"""Read release API credentials without persisting them or exposing CLI output."""
import json
import os
import re
import subprocess

SECRETS = set()


def validate(settings):
    if not isinstance(settings, dict) or set(settings) - {'github', 'gitlab'}:
        raise ValueError('Credentials must describe only github and gitlab providers.')
    for service, spec in settings.items():
        if not isinstance(spec, dict):
            raise ValueError('Invalid credential settings for '+service)
        if spec.get('provider', 'env') == 'env':
            if set(spec) - {'provider', 'variable'} or not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', spec.get('variable', service.upper()+'_TOKEN')):
                raise ValueError('Invalid environment credential reference for '+service)
        elif spec.get('provider') == '1password':
            if set(spec) - {'provider', 'reference', 'account', 'vault', 'item', 'field'} or not all(isinstance(v, str) for v in spec.values()):
                raise ValueError('Invalid 1Password settings for '+service)
            if spec.get('reference'):
                if not re.fullmatch(r'op://[^\r\n]+', spec['reference']):
                    raise ValueError('Invalid 1Password secret reference for '+service)
            elif not all(spec.get(key) for key in ('vault', 'item', 'field')):
                raise ValueError('Configure a 1Password reference or vault/item/field for '+service)
        else:
            raise ValueError('Unknown credential provider for '+service)


def token(service, cfg):
    settings = cfg.get('credentials', {})
    validate(settings)
    spec = settings.get(service, {})
    if spec.get('provider', 'env') == 'env':
        value = os.environ.get(spec.get('variable', service.upper()+'_TOKEN'), '')
    else:
        reference = spec.get('reference')
        if reference:
            command = ['op', 'read', '--no-newline', reference]
        else:
            command = ['op', 'item', 'get', spec['item'], '--vault', spec['vault'],
                       '--fields', 'label='+spec['field'], '--format', 'json', '--reveal']
        if spec.get('account'):
            command.extend(['--account', spec['account']])
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=120)
        except (OSError, subprocess.SubprocessError):
            raise ValueError('1Password lookup unavailable or cancelled for '+service+'; no secret output displayed.') from None
        if result.returncode:
            raise ValueError('1Password lookup failed or was cancelled for '+service+'; no secret output displayed.')
        if reference:
            value = result.stdout
        else:
            try:
                data = json.loads(result.stdout)
                fields = data if isinstance(data, list) else data.get('fields', [data])
                matches = [field for field in fields if field.get('label') == spec['field'] and field.get('value')]
                if len(matches) != 1:
                    raise ValueError()
                value = matches[0]['value']
            except (ValueError, KeyError, TypeError, AttributeError):
                raise ValueError('1Password field missing or ambiguous for '+service+'; use a copied secret reference.') from None
    if not isinstance(value, str) or not value.strip() or value.strip().startswith('op://') or any(c in value.strip() for c in '\r\n\0'):
        raise ValueError('Missing/unresolved '+service.upper()+'_TOKEN; configure its credential provider.')
    value = value.strip()
    SECRETS.add(value)
    return value


def known(cfg=None):
    result = set(SECRETS)
    names = {'GITHUB_TOKEN', 'GITLAB_TOKEN'}
    for service, spec in (cfg or {}).get('credentials', {}).items():
        if spec.get('provider', 'env') == 'env':
            names.add(spec.get('variable', service.upper()+'_TOKEN'))
    result.update(os.environ[name] for name in names if os.environ.get(name))
    return result


def build_environment(cfg):
    env = dict(os.environ, **cfg.get('env', {}))
    names = {'GITHUB_TOKEN', 'GITLAB_TOKEN', 'OP_SERVICE_ACCOUNT_TOKEN'}
    for service, spec in cfg.get('credentials', {}).items():
        if spec.get('provider', 'env') == 'env':
            names.add(spec.get('variable', service.upper()+'_TOKEN'))
    for name in list(env):
        if name in names or name.startswith('OP_SESSION_'):
            env.pop(name, None)
    return env


def redact(text):
    for value in sorted(known(), key=len, reverse=True):
        text = text.replace(value, '[redacted]')
    return text
