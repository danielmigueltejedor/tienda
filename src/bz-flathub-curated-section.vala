/*
 * bz-flathub-curated-section.vala
 *
 * Copyright 2026 Alexander Vanhee
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

using GLib.Intl;

[GtkTemplate (ui = "/os/nodalix/Store/bz-flathub-curated-section.ui")]
public class Bz.FlathubCuratedSection : Adw.Bin {
    private class ThemeInfo {
        public string title;
        public string subtitle;

        public ThemeInfo (string title, string subtitle) {
            this.title = title;
            this.subtitle = subtitle;
        }
    }

    private static HashTable<string, ThemeInfo> theme_map;

    static construct {
        theme_map = new HashTable<string, ThemeInfo> (str_hash, str_equal);

        theme_map.insert ("new-year-new-workflows",  new ThemeInfo (N_ ("New Year, New Workflows"),  N_ ("Apps to take notes, track tasks, and make plans")));
        theme_map.insert ("free-software-favorites", new ThemeInfo (N_ ("Staff Picks"),              N_ ("Apps we love right now")));
        theme_map.insert ("spring-creativity",       new ThemeInfo (N_ ("Get Creative"),             N_ ("Essential apps for drawing, editing photos, and digital design")));
        theme_map.insert ("fresh-desktop-releases",  new ThemeInfo (N_ ("Build Native Apps"),        N_ ("Develop apps that feel at home on your system")));
        theme_map.insert ("staying-connected",       new ThemeInfo (N_ ("Stay in Touch"),            N_ ("Essential collaboration and communication apps")));
        theme_map.insert ("summer-travel",           new ThemeInfo (N_ ("Ready for Vacations"),      N_ ("Maps, transit, weather, and trip planning apps")));
        theme_map.insert ("back-to-learning",        new ThemeInfo (N_ ("Smarter Every Day"),        N_ ("Apps to help you study, research, and write")));
        theme_map.insert ("winter-comforts",         new ThemeInfo (N_ ("Get Cozy"),                 N_ ("Great apps for reading, watching movies, and playing games")));
        theme_map.insert ("tools-for-developers",    new ThemeInfo (N_ ("Web Developer Essentials"), N_ ("Making websites is more fun with these apps")));
        theme_map.insert ("take-better-notes",       new ThemeInfo (N_ ("Take Better Notes"),        N_ ("Find your new favorite note taking tool")));
        theme_map.insert ("get-focused",             new ThemeInfo (N_ ("Get Focused"),              N_ ("Great apps for task management")));
        theme_map.insert ("make-some-noise",         new ThemeInfo (N_ ("Make Some Noise"),          N_ ("Apps for creating and producing music")));
        theme_map.insert ("get-to-work",             new ThemeInfo (N_ ("Get To Work"),              N_ ("Essential office apps")));
    }

    [GtkChild]
    private unowned Gtk.Box root_box;
    [GtkChild]
    private unowned Gtk.Label title_label;
    [GtkChild]
    private unowned Gtk.Label subtitle_label;
    [GtkChild]
    private unowned Gtk.SliceListModel slice_model;

    private string? applied_css_class;

    private Bz.FlathubCuratedSelection? _selection;
    public Bz.FlathubCuratedSelection? selection {
        get { return _selection; }
        set {
            if (value == null)
                return;
            _selection = value;

            rebuild ();
        }
    }

    private bool _compact = false;
    public bool compact {
        get { return _compact; }
        set {
            if (_compact == value)
                return;
            _compact = value;

            slice_model.size = calculate_max_length ();
        }
    }

    private uint calculate_max_length () {
        if (_compact)
            return 6;

        return _selection != null && _selection.apps != null &&
            _selection.apps.get_n_items () >= 12 ? 12 : 8;
    }

    private void rebuild () {
        string theme_key = _selection.theme_key;
        string slot = _selection.slot;
        unowned ThemeInfo? info = theme_key != null ? theme_map.lookup (theme_key) : null;
        bool has_info = info != null;

        slice_model.model = _selection.apps != null && _selection.map_factory != null
            ? _selection.map_factory.generate (_selection.apps) : null;

        slice_model.size = calculate_max_length ();

        title_label.set_label (has_info ? _ (info.title) : "");
        subtitle_label.set_label (has_info ? _ (info.subtitle) : "");
        title_label.set_visible (has_info);
        subtitle_label.set_visible (has_info);

        if (applied_css_class != null)
            root_box.remove_css_class (applied_css_class);
        applied_css_class = slot;
        if (applied_css_class != null)
            root_box.add_css_class (applied_css_class);
    }
}
