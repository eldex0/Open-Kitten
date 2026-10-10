//! Memory-safe privacy primitives for Open Kitten 2.0.

use std::ffi::{c_char, CStr};
use std::ptr;

const TRACKING_KEYS: &[&str] = &[
    "utm_", "fbclid", "gclid", "dclid", "msclkid", "mc_cid", "mc_eid",
    "igshid", "vero_id", "oly_anon_id", "oly_enc_id",
];

const TRACKING_HOSTS: &[&str] = &[
    "doubleclick.net",
    "google-analytics.com",
    "googletagmanager.com",
    "googlesyndication.com",
    "facebook.net",
];

pub fn is_tracking_host(host: &str) -> bool {
    let host = host.trim().trim_end_matches('.').to_ascii_lowercase();
    TRACKING_HOSTS
        .iter()
        .any(|blocked| host == *blocked || host.ends_with(&format!(".{blocked}")))
}

pub fn strip_tracking_parameters(url: &str) -> String {
    let Some((base, query_and_fragment)) = url.split_once('?') else {
        return url.to_owned();
    };
    let (query, fragment) = query_and_fragment
        .split_once('#')
        .map_or((query_and_fragment, ""), |(query, fragment)| (query, fragment));
    let kept: Vec<&str> = query
        .split('&')
        .filter(|pair| {
            let key = pair.split('=').next().unwrap_or_default().to_ascii_lowercase();
            !TRACKING_KEYS.iter().any(|prefix| key.starts_with(prefix))
        })
        .collect();
    let mut clean = base.to_owned();
    if !kept.is_empty() {
        clean.push('?');
        clean.push_str(&kept.join("&"));
    }
    if !fragment.is_empty() {
        clean.push('#');
        clean.push_str(fragment);
    }
    clean
}

unsafe fn c_string<'a>(value: *const c_char) -> Option<&'a str> {
    if value.is_null() {
        return None;
    }
    // SAFETY: the C API contract requires a valid NUL-terminated input pointer.
    CStr::from_ptr(value).to_str().ok()
}

#[no_mangle]
pub unsafe extern "C" fn kitten_is_tracking_host(host: *const c_char) -> bool {
    c_string(host).is_some_and(is_tracking_host)
}

/// Copies a cleaned URL into `output` and returns the required byte length,
/// excluding the terminating NUL. A null output pointer performs a size query.
#[no_mangle]
pub unsafe extern "C" fn kitten_strip_tracking_parameters(
    input: *const c_char,
    output: *mut c_char,
    capacity: usize,
) -> usize {
    let Some(input) = c_string(input) else {
        return 0;
    };
    let clean = strip_tracking_parameters(input);
    let bytes = clean.as_bytes();
    if output.is_null() || capacity == 0 {
        return bytes.len();
    }
    if capacity <= bytes.len() {
        return bytes.len();
    }
    // SAFETY: the caller provided at least bytes.len() + 1 writable bytes.
    unsafe {
        ptr::copy_nonoverlapping(bytes.as_ptr(), output.cast::<u8>(), bytes.len());
        *output.add(bytes.len()) = 0;
    }
    bytes.len()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn recognizes_tracking_hosts_without_substring_false_positives() {
        assert!(is_tracking_host("stats.doubleclick.net"));
        assert!(!is_tracking_host("doubleclick.net.example.org"));
    }

    #[test]
    fn removes_known_tracking_parameters_and_keeps_fragment() {
        assert_eq!(
            strip_tracking_parameters("https://example.org/p?utm_source=x&id=4#part"),
            "https://example.org/p?id=4#part"
        );
    }

    #[test]
    fn ffi_size_query_and_copy_are_consistent() {
        let input = CString::new("https://e.test/?gclid=x&ok=1").unwrap();
        let required = unsafe {
            kitten_strip_tracking_parameters(input.as_ptr(), ptr::null_mut(), 0)
        };
        let mut output = vec![0_i8; required + 1];
        assert_eq!(
            unsafe {
                kitten_strip_tracking_parameters(input.as_ptr(), output.as_mut_ptr(), output.len())
            },
            required
        );
        let result = unsafe { CStr::from_ptr(output.as_ptr()) }.to_str().unwrap();
        assert_eq!(result, "https://e.test/?ok=1");
    }
}
