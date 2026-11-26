"""Recognizable original RGB materials; theme-role metadata stays unchanged.

Only bundled authoring tools use this table. Third-party banks retain their
own RGB palette when the user selects Colourful icons.
"""
def colourful_palette(name):
    # dark outline, main material, highlight, two details; remaining RGB inks
    # are available to programs with richer artwork such as calendars.
    materials = {
        'preferences': (62, 132, 230), 'musicplayer': (172, 89, 224),
        'trash': (62, 181, 114), 'archiveextractor': (224, 158, 56),
        'notes': (241, 198, 67), 'timer': (226, 91, 84),
        'weather': (61, 164, 236), 'screenshot': (105, 93, 196),
        'screenmode': (48, 151, 222), 'places': (235, 173, 65),
        'saver': (61, 182, 172), 'alerts': (243, 181, 51),
        'shelf': (234, 146, 55), 'watch': (55, 171, 157),
        'logs': (70, 144, 219), 'calc': (237, 135, 53),
        'tooltypes': (127, 103, 210), 'amirocket': (224, 79, 65),
        'bench': (71, 155, 213), 'qr': (232, 239, 245),
        'moon': (104, 115, 179), 'colour': (211, 139, 67),
        'agenda': (223, 82, 86), 'pictures': (78, 170, 113),
    }
    case = materials[name.lower()]
    dark = tuple(max(14, c // 5) for c in case)
    return [(0, 0, 0), dark, case, (250, 248, 237),
            (48, 173, 235), (69, 214, 139), (240, 88, 97),
            (250, 204, 62), (126, 105, 223)] + [(0, 0, 0)] * 7
