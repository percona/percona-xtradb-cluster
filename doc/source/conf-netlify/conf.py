#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys, os
sys.path.append(os.path.abspath("../"))
from conf import *
extensions.append('sphinx_gitstamp')
extensions.append('sphinx_copybutton')
extensions.append('sphinx_tabs.tabs')
html_sidebars['**']=['globaltoc.html', 'searchbox.html', 'localtoc.html', 'logo-text.html']
html_theme = 'sphinx_material'
html_theme_options = {
    'base_url': 'http://bashtage.github.io/sphinx-material/',
    'repo_url': 'https://github.com/percona/percona-xtradb-cluster',
    'repo_name': 'percona/percona-xtradb-cluster',
    'color_accent': 'grey',
    'color_primary': 'orange',
    'google_analytics_account': 'UA-343802-3',
    'globaltoc_collapse': True,
    'version_dropdown': True,
    'version_dropdown_text': 'Versions',
    'version_info': {
        "5.6": "https://pxc-docs-56.netlify.app",
        "5.7": "https://pxc-docs-57.netlify.app/",
        "8.0": "https://pxc-docs-80.netlify.app/",
        "Latest": "https://pxc-docs-80.netlify.app/"
    },
}
html_logo = '../_static/images/percona-logo.svg'
html_favicon = '../_static/images/percona_favicon.ico'
pygments_style = 'emacs'
gitstamp_fmt = "%b %d, %Y"
copybutton_prompt_text = '$'
# Add any paths that contain templates here, relative to this directory.
templates_path = ['../_static/_templates/theme']
#html_last_updated_fmt = ''
