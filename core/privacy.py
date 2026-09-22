class PrivacyManager:

    def __init__(self, browser):
        self.browser = browser

    def clear_history(self):
        self.browser.storage.clear_history()

    def clear_cookies(self):
        profile = self.browser.profile

        profile.cookieStore().deleteAllCookies()

    def clear_cache(self):
        profile = self.browser.profile

        profile.clearHttpCache()

    def clear_all(self):
        self.clear_history()
        self.clear_cookies()
        self.clear_cache()