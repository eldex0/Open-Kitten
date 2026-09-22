import os
import re
import shutil


class ProfileManager:

    def __init__(self, base_data_dir):
        self.base_data_dir = base_data_dir

        self.profiles_dir = os.path.join(
            base_data_dir,
            "profiles"
        )

        os.makedirs(
            self.profiles_dir,
            exist_ok=True
        )

    def sanitize_name(self, name):
        name = name.strip()

        if not name:
            return "Default"

        name = re.sub(
            r'[<>:"/\\|?*]',
            "_",
            name
        )

        name = name[:50].rstrip(" .")
        if not name or name in {".", ".."}:
            return "Default"
        if name.split(".")[0].upper() in {
            "CON", "PRN", "AUX", "NUL",
            *(f"COM{i}" for i in range(1, 10)),
            *(f"LPT{i}" for i in range(1, 10)),
        }:
            name = "_" + name
        return name

    def get_profile_dir(self, name):
        name = self.sanitize_name(name)

        path = os.path.join(
            self.profiles_dir,
            name
        )
        root = os.path.realpath(self.profiles_dir)
        resolved = os.path.realpath(path)
        if os.path.commonpath([root, resolved]) != root or resolved == root:
            raise ValueError("Chemin de profil non autorisé")
        return path

    def create_profile(self, name):
        name = self.sanitize_name(name)

        path = self.get_profile_dir(name)

        os.makedirs(
            path,
            exist_ok=True
        )

        return path

    def delete_profile(self, name):
        name = self.sanitize_name(name)

        if name == "Default":
            return False

        path = self.get_profile_dir(name)

        if not os.path.exists(path):
            return False

        try:
            shutil.rmtree(path)
            return True

        except Exception:
            return False

    def list_profiles(self):
        if not os.path.exists(self.profiles_dir):
            return ["Default"]

        profiles = []

        for name in os.listdir(self.profiles_dir):
            path = os.path.join(
                self.profiles_dir,
                name
            )

            if os.path.isdir(path):
                profiles.append(name)

        if "Default" not in profiles:
            profiles.insert(0, "Default")

        return sorted(
            set(profiles),
            key=lambda x: x.lower()
        )
